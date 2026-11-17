#ifndef CLP_S_JSONSERIALIZER_HPP
#define CLP_S_JSONSERIALIZER_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

#include <clpp/ErrorCode.hpp>
#include <clpp/TextShape.hpp>

#include "ColumnReader.hpp"
#include "Utils.hpp"

namespace clp_s {
class JsonSerializer {
public:
    enum Op : uint8_t {
        BeginObject,
        EndObject,
        AddIntField,
        AddFloatField,
        AddFormattedFloatField,
        AddBoolField,
        AddStringField,
        AddArrayField,
        AddNullField,
        AddIntValue,
        AddFloatValue,
        AddFormattedFloatValue,
        AddBoolValue,
        AddStringValue,
        AddNullValue,
        BeginArray,
        EndArray,
        BeginUnnamedObject,
        BeginUnnamedArray,
        AddLiteralField,
        AddConstantStringField,
        AddReconstructedTextField,
        AddReconstructedTextValue,
    };

    static int64_t const cReservedLength = 4096;

    explicit JsonSerializer(int64_t reserved_length = cReservedLength) {
        m_json_string.reserve(reserved_length);
    }

    std::string& get_serialized_string() { return m_json_string; }

    /**
     * Resets the JsonSerializer for the next record.
     */
    void reset() {
        m_json_string.clear();
        m_op_list_index = 0;
        m_special_keys_index = 0;
        m_constant_strings_index = 0;
        m_compiled_shapes_index = 0;
    }

    /**
     * Clears the contents of the JsonSerializer to make room for a new set of operations.
     */
    void clear() {
        reset();
        m_op_list.clear();
        m_special_keys.clear();
        m_constant_strings.clear();
        m_compiled_shapes.clear();
    }

    void add_op(Op op) { m_op_list.push_back(op); }

    std::vector<Op>& get_op_list() { return m_op_list; }

    bool get_next_op(Op& op) {
        if (m_op_list_index < m_op_list.size()) {
            op = m_op_list[m_op_list_index++];
            return true;
        }
        return false;
    }

    void add_special_key(std::string_view const key) {
        std::string tmp;
        StringUtils::escape_json_string(tmp, key);
        m_special_keys.emplace_back(tmp);
    }

    void begin_object() {
        append_key();
        m_json_string += "{";
    }

    void begin_document() { m_json_string += "{"; }

    void end_document() {
        if ('{' != m_json_string.back()) {
            m_json_string[m_json_string.size() - 1] = '}';
        } else {
            m_json_string += '}';
        }
    }

    void end_object() {
        if (m_op_list[m_op_list_index - 2] != BeginObject
            && m_op_list[m_op_list_index - 2] != BeginUnnamedObject)
        {
            m_json_string.pop_back();
        }
        m_json_string += "},";
    }

    void begin_array_document() { m_json_string += "["; }

    void begin_array() {
        append_key();
        m_json_string += "[";
    }

    void end_array() {
        if (m_op_list[m_op_list_index - 2] != BeginArray
            && m_op_list[m_op_list_index - 2] != BeginUnnamedArray)
        {
            m_json_string.pop_back();
        }
        m_json_string += "],";
    }

    void append_key() { append_escaped_key(m_special_keys[m_special_keys_index++]); }

    void append_key(std::string_view const key) {
        m_json_string += "\"";
        StringUtils::escape_json_string(m_json_string, key);
        m_json_string += "\":";
    }

    void append_value(std::string_view const value) {
        m_json_string += value;
        m_json_string += ",";
    }

    void append_value_from_column(clp_s::BaseColumnReader* column, uint64_t cur_message) {
        column->extract_string_value_into_buffer(cur_message, m_json_string);
        m_json_string += ",";
    }

    void
    append_value_from_column_with_quotes(clp_s::BaseColumnReader* column, uint64_t cur_message) {
        m_json_string += "\"";
        column->extract_escaped_string_value_into_buffer(
                cur_message,
                m_json_string,
                m_string_value_escaper
        );
        m_json_string += "\",";
    }

    auto add_constant_string_field_op(std::string_view key, std::string_view value) -> void {
        add_op(Op::AddConstantStringField);
        add_special_key(key);
        std::string tmp;
        StringUtils::escape_json_string(tmp, value);
        m_constant_strings.emplace_back(tmp);
    }

    auto append_constant_string_field() -> void {
        append_key();
        m_json_string.push_back('"');
        m_json_string.append(m_constant_strings[m_constant_strings_index++]);
        m_json_string.append("\",");
    }

    /**
     * Adds the op that outputs the text reconstructed from `shape` as a field with key `key`,
     * compiling the shape for per-message reconstruction.
     * @param key The field's key.
     * @param shape The text shape to reconstruct from.
     * @param column_readers The readers of the shape's placeholders, in placeholder order.
     * @return A void result on success, or an error code indicating the failure:
     * - Forwards `compile_shape`'s return values.
     */
    [[nodiscard]] auto add_reconstructed_text_field_op(
            std::string_view key,
            clpp::TextShape<std::string_view> shape,
            std::span<BaseColumnReader* const> column_readers
    ) -> ystdlib::error_handling::Result<void> {
        YSTDLIB_ERROR_HANDLING_TRYV(compile_shape(shape, column_readers));
        add_op(Op::AddReconstructedTextField);
        add_special_key(key);
        return ystdlib::error_handling::success();
    }

    /**
     * Adds the op that outputs the text reconstructed from `shape` as an unnamed value, compiling
     * the shape for per-message reconstruction.
     * @param shape The text shape to reconstruct from.
     * @param column_readers The readers of the shape's placeholders, in placeholder order.
     * @return A void result on success, or an error code indicating the failure:
     * - Forwards `compile_shape`'s return values.
     */
    [[nodiscard]] auto add_reconstructed_text_value_op(
            clpp::TextShape<std::string_view> shape,
            std::span<BaseColumnReader* const> column_readers
    ) -> ystdlib::error_handling::Result<void> {
        YSTDLIB_ERROR_HANDLING_TRYV(compile_shape(shape, column_readers));
        add_op(Op::AddReconstructedTextValue);
        return ystdlib::error_handling::success();
    }

    /**
     * Appends the text reconstructed from the next compiled shape as a quoted JSON string.
     * @param message_index The index of the message to reconstruct the text for.
     */
    auto append_reconstructed_text_value(uint64_t message_index) -> void {
        auto const& shape{m_compiled_shapes.at(m_compiled_shapes_index++)};
        m_json_string.push_back('"');
        for (auto const& segment : shape) {
            if (auto const* literal{std::get_if<std::string>(&segment)}; nullptr != literal) {
                m_json_string.append(*literal);
                continue;
            }
            std::get<BaseColumnReader*>(segment)->extract_escaped_string_value_into_buffer(
                    message_index,
                    m_json_string,
                    m_string_value_escaper
            );
        }
        m_json_string.append("\",");
    }

private:
    // Types
    /**
     * A text shape compiled into segments for fast per-message reconstruction of the text it was
     * built from. Each segment is either:
     * - A `std::string` containing text that is safe to append to the output buffer (JSON-escaped
     *   and literal % unescaped).
     * - A `BaseColumnReader*` used to extract the value for that placeholder in the shape. The
     *   readers must outlive the compiled shape (i.e. remain valid until `clear()`).
     */
    using CompiledShape = std::vector<std::variant<std::string, BaseColumnReader*>>;

    // Methods
    void append_escaped_key(std::string_view const key) {
        m_json_string.push_back('"');
        m_json_string.append(key);
        m_json_string.append("\":");
    }

    /**
     * Compiles `shape` into a `CompiledShape` appended to `m_compiled_shapes`. Literal segments
     * are unescaped and JSON-escaped; the i-th placeholder segment is bound to the i-th reader in
     * `column_readers`.
     * @param shape The text shape to compile.
     * @param column_readers The readers of the shape's placeholders, in placeholder order.
     * @return A void result on success, or an error code indicating the failure:
     * - ClppErrorCodeEnum::Corrupt if the shape's placeholder count differs from
     *   `column_readers.size()`.
     * - Forwards `clpp::TextShape::segments`' return values.
     */
    [[nodiscard]] auto compile_shape(
            clpp::TextShape<std::string_view> shape,
            std::span<BaseColumnReader* const> column_readers
    ) -> ystdlib::error_handling::Result<void> {
        using Segment = clpp::TextShape<std::string_view>::Segment;
        CompiledShape compiled_shape;
        size_t placeholder_idx{0};
        for (auto const& seg : YSTDLIB_ERROR_HANDLING_TRYX(shape.segments())) {
            switch (seg.type) {
                case Segment::Type::Literal: {
                    std::string json_escaped;
                    StringUtils::escape_json_string(
                            json_escaped,
                            clpp::TextShape<std::string_view>::unescape_literal_text(seg.text)
                    );
                    compiled_shape.emplace_back(std::move(json_escaped));
                    break;
                }
                case Segment::Type::Placeholder: {
                    if (placeholder_idx >= column_readers.size()) {
                        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Corrupt};
                    }
                    compiled_shape.emplace_back(column_readers[placeholder_idx]);
                    ++placeholder_idx;
                    break;
                }
                default: {
                    return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Corrupt};
                }
            }
        }
        if (placeholder_idx != column_readers.size()) {
            return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Corrupt};
        }
        m_compiled_shapes.emplace_back(std::move(compiled_shape));
        return ystdlib::error_handling::success();
    }

    // Data members
    std::string m_json_string;
    std::vector<Op> m_op_list;
    std::vector<std::string> m_special_keys;
    SimdJsonStringEscaper m_string_value_escaper;

    size_t m_op_list_index{0};
    size_t m_special_keys_index{0};

    std::vector<std::string> m_constant_strings;
    size_t m_constant_strings_index{0};

    std::vector<CompiledShape> m_compiled_shapes;
    size_t m_compiled_shapes_index{0};
};
}  // namespace clp_s

#endif  // CLP_S_JSONSERIALIZER_HPP
