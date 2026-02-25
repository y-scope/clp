#ifndef CLP_S_COLUMNREADER_HPP
#define CLP_S_COLUMNREADER_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>

#include <clp_s/BufferViewReader.hpp>
#include <clp_s/Defs.hpp>
#include <clp_s/DictionaryReader.hpp>
#include <clp_s/ErrorCode.hpp>
#include <clp_s/FloatFormatEncoding.hpp>
#include <clp_s/SchemaTree.hpp>
#include <clp_s/TimestampDictionaryReader.hpp>
#include <clp_s/TraceableException.hpp>
#include <clp_s/Utils.hpp>

namespace clp_s {
class BaseColumnReader {
public:
    class OperationFailed : public TraceableException {
    public:
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    BaseColumnReader(int32_t id) : m_id(id) {}

    virtual ~BaseColumnReader() = default;
    BaseColumnReader(BaseColumnReader const&) = default;
    auto operator=(BaseColumnReader const&) -> BaseColumnReader& = default;
    BaseColumnReader(BaseColumnReader&&) noexcept = default;
    auto operator=(BaseColumnReader&&) noexcept -> BaseColumnReader& = default;

    /**
     * Reads the column from a shared buffer.
     * @param buffer
     * @param num_messages
     */
    virtual auto load(BufferViewReader& reader, uint64_t num_messages) -> void = 0;

    [[nodiscard]] auto get_id() const -> int32_t { return m_id; }

    virtual auto get_type() -> NodeType { return NodeType::Unknown; }

    /**
     * Extracts a value of the column
     * @param cur_message
     * @return Value
     */
    virtual auto extract_value(uint64_t cur_message)
            -> std::variant<int64_t, double, std::string, uint8_t>
            = 0;

    /**
     * Extracts a value from the column and serializes it into a provided buffer as a string.
     * @param cur_message
     * @param buffer
     */
    virtual auto extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer) -> void
            = 0;

    /**
     * Extracts a value from the column, escapes it, and serializes it into a provided buffer as a
     * string.
     * @param cur_message
     * @param buffer
     */
    virtual auto extract_escaped_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
            -> void {
        extract_string_value_into_buffer(cur_message, buffer);
    }

private:
    int32_t m_id;
};

class Int64ColumnReader : public BaseColumnReader {
public:
    explicit Int64ColumnReader(int32_t id) : BaseColumnReader(id) {}

    // Methods inherited from BaseColumnReader
    auto load(BufferViewReader& reader, uint64_t num_messages) -> void override;

    auto get_type() -> NodeType override { return NodeType::Integer; }

    auto extract_value(uint64_t cur_message)
            -> std::variant<int64_t, double, std::string, uint8_t> override;

    auto extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
            -> void override;

private:
    UnalignedMemSpan<int64_t> m_values;
};

class DeltaEncodedInt64ColumnReader : public BaseColumnReader {
public:
    explicit DeltaEncodedInt64ColumnReader(int32_t id) : BaseColumnReader(id) {}

    // Methods inherited from BaseColumnReader
    auto load(BufferViewReader& reader, uint64_t num_messages) -> void override;

    auto get_type() -> NodeType override { return NodeType::DeltaInteger; }

    auto extract_value(uint64_t cur_message)
            -> std::variant<int64_t, double, std::string, uint8_t> override;

    auto extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
            -> void override;

    /**
     * Gets the value stored at a given index by summing up the stored deltas between the requested
     * index and the last requested index.
     * @param idx
     * @return The value stored at the requested index.
     */
    [[nodiscard]] auto get_value_at_idx(size_t idx) -> int64_t;

private:
    UnalignedMemSpan<int64_t> m_values;
    int64_t m_cur_value{};
    size_t m_cur_idx{};
};

class FloatColumnReader : public BaseColumnReader {
public:
    explicit FloatColumnReader(int32_t id) : BaseColumnReader(id) {}

    // Methods inherited from BaseColumnReader
    auto load(BufferViewReader& reader, uint64_t num_messages) -> void override;

    auto get_type() -> NodeType override { return NodeType::Float; }

    auto extract_value(uint64_t cur_message)
            -> std::variant<int64_t, double, std::string, uint8_t> override;

    auto extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
            -> void override;

private:
    UnalignedMemSpan<double> m_values;
};

class FormattedFloatColumnReader : public BaseColumnReader {
public:
    explicit FormattedFloatColumnReader(int32_t id) : BaseColumnReader(id) {}

    // Methods inherited from BaseColumnReader
    auto load(BufferViewReader& reader, uint64_t num_messages) -> void override;

    auto get_type() -> NodeType override { return NodeType::FormattedFloat; }

    auto extract_value(uint64_t cur_message)
            -> std::variant<int64_t, double, std::string, uint8_t> override;

    /**
     * Appends the floating point value to the buffer in its original format by decoding the stored
     * format information.
     *
     * @param cur_message
     * @param buffer
     */
    auto extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
            -> void override;

private:
    UnalignedMemSpan<double> m_values;
    UnalignedMemSpan<float_format_t> m_formats;
};

class DictionaryFloatColumnReader : public BaseColumnReader {
public:
    explicit DictionaryFloatColumnReader(
            int32_t id,
            std::shared_ptr<VariableDictionaryReader> var_dict
    )
            : BaseColumnReader(id),
              m_var_dict{std::move(var_dict)} {}

    // Methods inherited from BaseColumnReader
    auto load(BufferViewReader& reader, uint64_t num_messages) -> void override;

    auto get_type() -> NodeType override { return NodeType::DictionaryFloat; }

    auto extract_value(uint64_t cur_message)
            -> std::variant<int64_t, double, std::string, uint8_t> override;

    auto extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
            -> void override;

private:
    std::shared_ptr<VariableDictionaryReader> m_var_dict;
    UnalignedMemSpan<variable_dictionary_id_t> m_var_dict_ids;
};

class BooleanColumnReader : public BaseColumnReader {
public:
    explicit BooleanColumnReader(int32_t id) : BaseColumnReader(id) {}

    // Methods inherited from BaseColumnReader
    auto load(BufferViewReader& reader, uint64_t num_messages) -> void override;

    auto get_type() -> NodeType override { return NodeType::Boolean; }

    auto extract_value(uint64_t cur_message)
            -> std::variant<int64_t, double, std::string, uint8_t> override;

    auto extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
            -> void override;

private:
    UnalignedMemSpan<uint8_t> m_values;
};

class ClpStringColumnReader : public BaseColumnReader {
public:
    ClpStringColumnReader(
            int32_t id,
            std::shared_ptr<VariableDictionaryReader> var_dict,
            std::shared_ptr<LogTypeDictionaryReader> log_dict,
            bool is_array = false
    )
            : BaseColumnReader(id),
              m_var_dict(std::move(var_dict)),
              m_log_dict(std::move(log_dict)),
              m_is_array(is_array) /*, encoded_vars_index_(0)*/ {}

    // Methods inherited from BaseColumnReader
    auto load(BufferViewReader& reader, uint64_t num_messages) -> void override;

    auto get_type() -> NodeType override {
        return m_is_array ? NodeType::UnstructuredArray : NodeType::ClpString;
    }

    auto extract_value(uint64_t cur_message)
            -> std::variant<int64_t, double, std::string, uint8_t> override;

    auto extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
            -> void override;

    auto extract_escaped_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
            -> void override;

    /**
     * Gets the encoded id of the variable
     * @param cur_message
     * @return The encoded logtype id
     */
    auto get_encoded_id(uint64_t cur_message) -> int64_t;

    /**
     * Gets the encoded variables
     * @param cur_message
     * @return Encoded variables in a span
     */
    auto get_encoded_vars(uint64_t cur_message) -> UnalignedMemSpan<int64_t>;

private:
    std::shared_ptr<VariableDictionaryReader> m_var_dict;
    std::shared_ptr<LogTypeDictionaryReader> m_log_dict;

    UnalignedMemSpan<uint64_t> m_logtypes;
    UnalignedMemSpan<int64_t> m_encoded_vars;

    bool m_is_array;
};

class VariableStringColumnReader : public BaseColumnReader {
public:
    VariableStringColumnReader(int32_t id, std::shared_ptr<VariableDictionaryReader> var_dict)
            : BaseColumnReader(id),
              m_var_dict(std::move(var_dict)) {}

    // Methods inherited from BaseColumnReader
    auto load(BufferViewReader& reader, uint64_t num_messages) -> void override;

    auto get_type() -> NodeType override { return NodeType::VarString; }

    auto extract_value(uint64_t cur_message)
            -> std::variant<int64_t, double, std::string, uint8_t> override;

    auto extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
            -> void override;

    auto extract_escaped_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
            -> void override;

    /**
     * Gets the encoded id of the variable
     * @param cur_message
     * @return The encoded logtype id
     */
    auto get_variable_id(uint64_t cur_message) -> uint64_t;

private:
    std::shared_ptr<VariableDictionaryReader> m_var_dict;

    UnalignedMemSpan<uint64_t> m_variables;
};

class DeprecatedDateStringColumnReader : public BaseColumnReader {
public:
    // Constructor
    DeprecatedDateStringColumnReader(
            int32_t id,
            std::shared_ptr<TimestampDictionaryReader> timestamp_dict
    )
            : BaseColumnReader(id),
              m_timestamp_dict(std::move(timestamp_dict)) {}

    // Methods inherited from BaseColumnReader
    auto load(BufferViewReader& reader, uint64_t num_messages) -> void override;

    auto get_type() -> NodeType override { return NodeType::DeprecatedDateString; }

    auto extract_value(uint64_t cur_message)
            -> std::variant<int64_t, double, std::string, uint8_t> override;

    auto extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
            -> void override;

    /**
     * @param cur_message
     * @return The encoded time in epoch time
     */
    auto get_encoded_time(uint64_t cur_message) -> epochtime_t;

private:
    std::shared_ptr<TimestampDictionaryReader> m_timestamp_dict;

    UnalignedMemSpan<int64_t> m_timestamps;
    UnalignedMemSpan<int64_t> m_timestamp_encodings;
};

class TimestampColumnReader : public BaseColumnReader {
public:
    TimestampColumnReader(int32_t id, std::shared_ptr<TimestampDictionaryReader> timestamp_dict)
            : BaseColumnReader{id},
              m_timestamp_dict{std::move(timestamp_dict)},
              m_timestamps{id} {}

    // Methods inherited from BaseColumnReader
    auto load(BufferViewReader& reader, uint64_t num_messages) -> void override;

    auto get_type() -> NodeType override { return NodeType::Timestamp; }

    auto extract_value(uint64_t cur_message)
            -> std::variant<int64_t, double, std::string, uint8_t> override;

    auto extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer)
            -> void override;

    /**
     * @param cur_message
     * @return The encoded time in epoch nanoseconds.
     */
    [[nodiscard]] auto get_encoded_time(uint64_t cur_message) -> epochtime_t;

private:
    std::shared_ptr<TimestampDictionaryReader> m_timestamp_dict;

    DeltaEncodedInt64ColumnReader m_timestamps;
    UnalignedMemSpan<uint64_t> m_timestamp_encodings;
};
}  // namespace clp_s

#endif  // CLP_S_COLUMNREADER_HPP
