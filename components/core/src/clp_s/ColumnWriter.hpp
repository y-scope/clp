#ifndef CLP_S_COLUMNWRITER_HPP
#define CLP_S_COLUMNWRITER_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include <clp/Defs.h>
#include <clp_s/DictionaryEntry.hpp>
#include <clp_s/DictionaryWriter.hpp>
#include <clp_s/FloatFormatEncoding.hpp>
#include <clp_s/ParsedMessage.hpp>
#include <clp_s/ZstdCompressor.hpp>

namespace clp_s {
class BaseColumnWriter {
public:
    // Constructors
    BaseColumnWriter() = default;

    BaseColumnWriter(BaseColumnWriter const&) = default;
    BaseColumnWriter(BaseColumnWriter&&) noexcept = default;

    // Destructors
    virtual ~BaseColumnWriter() = default;

    // Operators
    auto operator=(BaseColumnWriter const&) -> BaseColumnWriter& = default;
    auto operator=(BaseColumnWriter&&) noexcept -> BaseColumnWriter& = default;

    // Methods
    /**
     * Adds a value to the column
     * @param value
     * @return the size of the encoded data appended to this column in bytes
     */
    virtual auto add_value(ParsedMessage::variable_t& value) -> size_t = 0;

    /**
     * Stores the column to a compressed file.
     * @param compressor
     */
    virtual auto store(ZstdCompressor& compressor) -> void = 0;

    /**
     * Returns the total size of the header data that will be written to the compressor. This header
     * size plus the sum of sizes returned by add_value is equal to the total size of data that will
     * be written to the compressor in bytes.
     *
     * @return the total size of header data that will be written to the compressor in bytes
     */
    [[nodiscard]] virtual auto get_total_header_size() const -> size_t { return 0; }
};

class Int64ColumnWriter : public BaseColumnWriter {
public:
    // Methods inherited from BaseColumnWriter
    auto add_value(ParsedMessage::variable_t& value) -> size_t override;

    auto store(ZstdCompressor& compressor) -> void override;

private:
    std::vector<int64_t> m_values;
};

class DeltaEncodedInt64ColumnWriter : public BaseColumnWriter {
public:
    // Methods
    [[nodiscard]] auto add_value(int64_t value) -> size_t;

    // Methods inherited from BaseColumnWriter
    auto add_value(ParsedMessage::variable_t& value) -> size_t override;

    auto store(ZstdCompressor& compressor) -> void override;

private:
    std::vector<int64_t> m_values;
    int64_t m_cur{};
};

class FloatColumnWriter : public BaseColumnWriter {
public:
    // Methods inherited from BaseColumnWriter
    auto add_value(ParsedMessage::variable_t& value) -> size_t override;

    auto store(ZstdCompressor& compressor) -> void override;

private:
    std::vector<double> m_values;
};

class FormattedFloatColumnWriter : public BaseColumnWriter {
public:
    // Methods inherited from BaseColumnWriter
    auto add_value(ParsedMessage::variable_t& value) -> size_t override;

    auto store(ZstdCompressor& compressor) -> void override;

private:
    std::vector<double> m_values;
    std::vector<float_format_t> m_formats;
};

class DictionaryFloatColumnWriter : public BaseColumnWriter {
public:
    // Constructor
    DictionaryFloatColumnWriter(std::shared_ptr<VariableDictionaryWriter> var_dict)
            : m_var_dict(std::move(var_dict)) {}

    // Methods inherited from BaseColumnWriter
    auto add_value(ParsedMessage::variable_t& value) -> size_t override;

    auto store(ZstdCompressor& compressor) -> void override;

private:
    std::shared_ptr<VariableDictionaryWriter> m_var_dict;
    std::vector<clp::variable_dictionary_id_t> m_var_dict_ids;
};

class BooleanColumnWriter : public BaseColumnWriter {
public:
    // Methods inherited from BaseColumnWriter
    auto add_value(ParsedMessage::variable_t& value) -> size_t override;

    auto store(ZstdCompressor& compressor) -> void override;

private:
    std::vector<uint8_t> m_values;
};

class ClpStringColumnWriter : public BaseColumnWriter {
public:
    // Types
    using encoded_log_dict_id_t = uint64_t;

    // Constructor
    ClpStringColumnWriter(
            std::shared_ptr<VariableDictionaryWriter> var_dict,
            std::shared_ptr<LogTypeDictionaryWriter> log_dict
    )
            : m_var_dict(std::move(var_dict)),
              m_log_dict(std::move(log_dict)) {}

    // Methods inherited from BaseColumnWriter
    auto add_value(ParsedMessage::variable_t& value) -> size_t override;

    auto store(ZstdCompressor& compressor) -> void override;

    [[nodiscard]] auto get_total_header_size() const -> size_t override { return sizeof(size_t); }

    /**
     * @param encoded_id
     * @return the encoded log dict id
     */
    static auto get_encoded_log_dict_id(encoded_log_dict_id_t encoded_id)
            -> clp::logtype_dictionary_id_t {
        return static_cast<clp::logtype_dictionary_id_t>(encoded_id & cLogDictIdMask);
    }

    /**
     * @param encoded_id
     * @return The encoded offset
     */
    static auto get_encoded_offset(encoded_log_dict_id_t encoded_id) -> uint64_t {
        return (encoded_id & cOffsetMask) >> cOffsetBitPosition;
    }

private:
    /**
     * Encodes a log dict id
     * @param id
     * @param offset
     * @return The encoded log dict id
     */
    static auto encode_log_dict_id(clp::logtype_dictionary_id_t id, uint64_t offset)
            -> encoded_log_dict_id_t {
        return static_cast<encoded_log_dict_id_t>(id) | (offset << cOffsetBitPosition);
    }

    static constexpr int cOffsetBitPosition = 24;
    static constexpr uint64_t cLogDictIdMask = (1ULL << cOffsetBitPosition) - 1;
    static constexpr uint64_t cOffsetMask = ~cLogDictIdMask;

    std::shared_ptr<VariableDictionaryWriter> m_var_dict;
    std::shared_ptr<LogTypeDictionaryWriter> m_log_dict;
    LogTypeDictionaryEntry m_logtype_entry;

    std::vector<encoded_log_dict_id_t> m_logtypes;
    std::vector<clp::encoded_variable_t> m_encoded_vars;
};

class VariableStringColumnWriter : public BaseColumnWriter {
public:
    // Constructors
    VariableStringColumnWriter(std::shared_ptr<VariableDictionaryWriter> var_dict)
            : m_var_dict(std::move(var_dict)) {}

    // Methods inherited from BaseColumnWriter
    auto add_value(ParsedMessage::variable_t& value) -> size_t override;

    auto store(ZstdCompressor& compressor) -> void override;

private:
    std::shared_ptr<VariableDictionaryWriter> m_var_dict;
    std::vector<clp::variable_dictionary_id_t> m_var_dict_ids;
};

class TimestampColumnWriter : public BaseColumnWriter {
public:
    // Methods inherited from BaseColumnWriter
    auto add_value(ParsedMessage::variable_t& value) -> size_t override;

    auto store(ZstdCompressor& compressor) -> void override;

private:
    DeltaEncodedInt64ColumnWriter m_timestamps;
    std::vector<uint64_t> m_timestamp_encodings;
};
}  // namespace clp_s

#endif  // CLP_S_COLUMNWRITER_HPP
