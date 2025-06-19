#ifndef CLP_S_COLUMNWRITER_HPP
#define CLP_S_COLUMNWRITER_HPP

#include <utility>
#include <variant>

#include "DictionaryWriter.hpp"
#include "FileWriter.hpp"
#include "ParsedMessage.hpp"
#include "TimestampDictionaryWriter.hpp"
#include "VariableEncoder.hpp"
#include "ZstdCompressor.hpp"

namespace clp_s {
class BaseColumnWriter {
public:
    // Constructor
    explicit BaseColumnWriter(int32_t id) : m_id(id) {}

    // Destructor
    virtual ~BaseColumnWriter() = default;

    /**
     * Adds a value to the column
     * @param value
     * @return the size of the encoded data appended to this column in bytes
     */
    virtual size_t add_value(ParsedMessage::variable_t& value) = 0;

    /**
     * Stores the column to a compressed file.
     * @param compressor
     */
    virtual void store(ZstdCompressor& compressor) = 0;

    /**
     * Returns the total size of the header data that will be written to the compressor. This header
     * size plus the sum of sizes returned by add_value is equal to the total size of data that will
     * be written to the compressor in bytes.
     *
     * @return the total size of header data that will be written to the compressor in bytes
     */
    virtual size_t get_total_header_size() const { return 0; }

protected:
    int32_t m_id;
};

class Int64ColumnWriter : public BaseColumnWriter {
public:
    // Constructor
    explicit Int64ColumnWriter(int32_t id) : BaseColumnWriter(id) {}

    // Destructor
    ~Int64ColumnWriter() override = default;

    // Methods inherited from BaseColumnWriter
    size_t add_value(ParsedMessage::variable_t& value) override;

    void store(ZstdCompressor& compressor) override;

private:
    std::vector<int64_t> m_values;
};

class DeltaEncodedInt64ColumnWriter : public BaseColumnWriter {
public:
    // Constructor
    explicit DeltaEncodedInt64ColumnWriter(int32_t id) : BaseColumnWriter(id) {}

    // Destructor
    ~DeltaEncodedInt64ColumnWriter() override = default;

    // Methods inherited from BaseColumnWriter
    size_t add_value(ParsedMessage::variable_t& value) override;

    void store(ZstdCompressor& compressor) override;

private:
    std::vector<int64_t> m_values;
    int64_t m_cur{};
};

class FloatColumnWriter : public BaseColumnWriter {
public:
    // Constructor
    explicit FloatColumnWriter(int32_t id) : BaseColumnWriter(id) {}

    // Destructor
    ~FloatColumnWriter() override = default;

    // Methods inherited from BaseColumnWriter
    size_t add_value(ParsedMessage::variable_t& value) override;

    void store(ZstdCompressor& compressor) override;

private:
    std::vector<double> m_values;
};

class BooleanColumnWriter : public BaseColumnWriter {
public:
    // Constructor
    explicit BooleanColumnWriter(int32_t id) : BaseColumnWriter(id) {}

    // Destructor
    ~BooleanColumnWriter() override = default;

    // Methods inherited from BaseColumnWriter
    size_t add_value(ParsedMessage::variable_t& value) override;

    void store(ZstdCompressor& compressor) override;

private:
    std::vector<uint8_t> m_values;
};

class ClpStringColumnWriter : public BaseColumnWriter {
public:
    // Constructor
    ClpStringColumnWriter(
            int32_t id,
            std::shared_ptr<VariableDictionaryWriter> var_dict,
            std::shared_ptr<LogTypeDictionaryWriter> log_dict
    )
            : BaseColumnWriter(id),
              m_var_dict(std::move(var_dict)),
              m_log_dict(std::move(log_dict)) {}

    // Destructor
    ~ClpStringColumnWriter() override = default;

    // Methods inherited from BaseColumnWriter
    size_t add_value(ParsedMessage::variable_t& value) override;

    void store(ZstdCompressor& compressor) override;

    size_t get_total_header_size() const override { return sizeof(size_t); }

    /**
     * @param encoded_id
     * @return the encoded log dict id
     */
    static int64_t get_encoded_log_dict_id(uint64_t encoded_id) {
        return (int64_t)encoded_id & cLogDictIdMask;
    }

    /**
     * @param encoded_id
     * @return The encoded offset
     */
    static int64_t get_encoded_offset(uint64_t encoded_id) {
        return ((int64_t)encoded_id & cOffsetMask) >> cOffsetBitPosition;
    }

private:
    /**
     * Encodes a log dict id
     * @param id
     * @param offset
     * @return The encoded log dict id
     */
    static int64_t encode_log_dict_id(uint64_t id, uint64_t offset) {
        return ((int64_t)id) | ((int64_t)offset) << cOffsetBitPosition;
    }

    static constexpr int cOffsetBitPosition = 24;
    static constexpr int64_t cLogDictIdMask = ~(-1ULL << cOffsetBitPosition);
    static constexpr int64_t cOffsetMask = ~cLogDictIdMask;

    std::shared_ptr<VariableDictionaryWriter> m_var_dict;
    std::shared_ptr<LogTypeDictionaryWriter> m_log_dict;
    LogTypeDictionaryEntry m_logtype_entry;

    std::vector<int64_t> m_logtypes;
    std::vector<int64_t> m_encoded_vars;
};

class VariableStringColumnWriter : public BaseColumnWriter {
public:
    // Constructor
    VariableStringColumnWriter(int32_t id, std::shared_ptr<VariableDictionaryWriter> var_dict)
            : BaseColumnWriter(id),
              m_var_dict(std::move(var_dict)) {}

    // Destructor
    ~VariableStringColumnWriter() override = default;

    // Methods inherited from BaseColumnWriter
    size_t add_value(ParsedMessage::variable_t& value) override;

    void store(ZstdCompressor& compressor) override;

private:
    std::shared_ptr<VariableDictionaryWriter> m_var_dict;
    std::vector<int64_t> m_variables;
};

class DateStringColumnWriter : public BaseColumnWriter {
public:
    // Constructor
    explicit DateStringColumnWriter(int32_t id) : BaseColumnWriter(id) {}

    // Destructor
    ~DateStringColumnWriter() override = default;

    // Methods inherited from BaseColumnWriter
    size_t add_value(ParsedMessage::variable_t& value) override;

    void store(ZstdCompressor& compressor) override;

private:
    std::vector<int64_t> m_timestamps;
    std::vector<int64_t> m_timestamp_encodings;
};
}  // namespace clp_s

#endif  // CLP_S_COLUMNWRITER_HPP
