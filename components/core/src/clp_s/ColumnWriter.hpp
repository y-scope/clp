#ifndef CLP_S_COLUMNWRITER_HPP
#define CLP_S_COLUMNWRITER_HPP

#include <utility>
#include <variant>

#include <simdjson.h>

#include "DictionaryWriter.hpp"
#include "FileWriter.hpp"
#include "TimestampDictionaryWriter.hpp"
#include "VariableEncoder.hpp"
#include "ZstdCompressor.hpp"

using namespace simdjson;

namespace clp_s {
class BaseColumnWriter {
public:
    // Constructor
    explicit BaseColumnWriter(std::string name, int32_t id) : m_name(std::move(name)), m_id(id) {}

    // Destructor
    virtual ~BaseColumnWriter() = default;

    /**
     * Adds a value to the column
     * @param value
     * @param size
     */
    virtual void add_value(std::variant<int64_t, double, std::string, bool>& value, size_t& size)
            = 0;

    /**
     * Stores the column to a compressed file
     * @param compressor
     */
    virtual void store(ZstdCompressor& compressor) = 0;

    /**
     * @return Name of the column
     */
    std::string get_name() { return m_name; }

protected:
    std::string m_name;
    int32_t m_id;
};

class Int64ColumnWriter : public BaseColumnWriter {
public:
    // Constructor
    explicit Int64ColumnWriter(std::string name, int32_t id)
            : BaseColumnWriter(std::move(name), id) {}

    // Destructor
    ~Int64ColumnWriter() override = default;

    // Methods inherited from BaseColumnWriter
    void add_value(std::variant<int64_t, double, std::string, bool>& value, size_t& size) override;

    void store(ZstdCompressor& compressor) override;

private:
    std::vector<int64_t> m_values;
};

class FloatColumnWriter : public BaseColumnWriter {
public:
    // Constructor
    explicit FloatColumnWriter(std::string name, int32_t id)
            : BaseColumnWriter(std::move(name), id) {}

    // Destructor
    ~FloatColumnWriter() override = default;

    // Methods inherited from BaseColumnWriter
    void add_value(std::variant<int64_t, double, std::string, bool>& value, size_t& size) override;

    void store(ZstdCompressor& compressor) override;

private:
    std::vector<double> m_values;
};

class BooleanColumnWriter : public BaseColumnWriter {
public:
    // Constructor
    explicit BooleanColumnWriter(std::string name, int32_t id)
            : BaseColumnWriter(std::move(name), id) {}

    // Destructor
    ~BooleanColumnWriter() override = default;

    // Methods inherited from BaseColumnWriter
    void add_value(std::variant<int64_t, double, std::string, bool>& value, size_t& size) override;

    void store(ZstdCompressor& compressor) override;

private:
    std::vector<uint8_t> m_values;
};

class ClpStringColumnWriter : public BaseColumnWriter {
public:
    // Constructor
    ClpStringColumnWriter(
            std::string const& name,
            int32_t id,
            std::shared_ptr<VariableDictionaryWriter> var_dict,
            std::shared_ptr<LogTypeDictionaryWriter> log_dict
    )
            : BaseColumnWriter(name, id),
              m_var_dict(std::move(var_dict)),
              m_log_dict(std::move(log_dict)) {}

    // Destructor
    ~ClpStringColumnWriter() override = default;

    // Methods inherited from BaseColumnWriter
    void add_value(std::variant<int64_t, double, std::string, bool>& value, size_t& size) override;

    void store(ZstdCompressor& compressor) override;

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
    VariableStringColumnWriter(
            std::string const& name,
            int32_t id,
            std::shared_ptr<VariableDictionaryWriter> var_dict
    )
            : BaseColumnWriter(name, id),
              m_var_dict(std::move(var_dict)) {}

    // Destructor
    ~VariableStringColumnWriter() override = default;

    // Methods inherited from BaseColumnWriter
    void add_value(std::variant<int64_t, double, std::string, bool>& value, size_t& size) override;

    void store(ZstdCompressor& compressor) override;

private:
    std::shared_ptr<VariableDictionaryWriter> m_var_dict;
    std::vector<int64_t> m_variables;
};

class DateStringColumnWriter : public BaseColumnWriter {
public:
    // Constructor
    DateStringColumnWriter(
            std::string const& name,
            int32_t id,
            std::shared_ptr<TimestampDictionaryWriter> timestamp_dict
    )
            : BaseColumnWriter(name, id),
              m_timestamp_dict(std::move(timestamp_dict)) {}

    // Destructor
    ~DateStringColumnWriter() override = default;

    // Methods inherited from BaseColumnWriter
    void add_value(std::variant<int64_t, double, std::string, bool>& value, size_t& size) override;

    void store(ZstdCompressor& compressor) override;

private:
    std::shared_ptr<TimestampDictionaryWriter> m_timestamp_dict;

    std::vector<int64_t> m_timestamps;
    std::vector<int64_t> m_timestamp_encodings;
};

class FloatDateStringColumnWriter : public BaseColumnWriter {
public:
    // Constructor
    FloatDateStringColumnWriter(
            std::string const& name,
            int32_t id,
            std::shared_ptr<TimestampDictionaryWriter> timestamp_dict
    )
            : BaseColumnWriter(name, id),
              m_timestamp_dict(std::move(timestamp_dict)) {}

    // Destructor
    ~FloatDateStringColumnWriter() override = default;

    // Methods inherited from BaseColumnWriter
    void add_value(std::variant<int64_t, double, std::string, bool>& value, size_t& size) override;

    void store(ZstdCompressor& compressor) override;

private:
    std::shared_ptr<TimestampDictionaryWriter> m_timestamp_dict;

    std::vector<double> m_timestamps;
};
}  // namespace clp_s

#endif  // CLP_S_COLUMNWRITER_HPP
