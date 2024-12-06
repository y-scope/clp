#ifndef CLP_S_COLUMNREADER_HPP
#define CLP_S_COLUMNREADER_HPP

#include <string>
#include <variant>

#include "BufferViewReader.hpp"
#include "DictionaryReader.hpp"
#include "SchemaTree.hpp"
#include "TimestampDictionaryReader.hpp"
#include "Utils.hpp"
#include "ZstdDecompressor.hpp"

namespace clp_s {
class BaseColumnReader {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructor
    BaseColumnReader(int32_t id) : m_id(id) {}

    // Destructor
    virtual ~BaseColumnReader() = default;

    /**
     * Reads the column from a shared buffer.
     * @param buffer
     * @param num_messages
     */
    virtual void load(BufferViewReader& reader, uint64_t num_messages) = 0;

    int32_t get_id() const { return m_id; }

    virtual NodeType get_type() { return NodeType::Unknown; }

    /**
     * Extracts a value of the column
     * @param cur_message
     * @return Value
     */
    virtual std::variant<int64_t, double, std::string, uint8_t> extract_value(uint64_t cur_message)
            = 0;

    /**
     * Extracts a value from the column and serializes it into a provided buffer as a string.
     * @param cur_message
     * @param buffer
     */
    virtual void extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer) = 0;

    /**
     * Extracts a value from the column, escapes it, and serializes it into a provided buffer as a
     * string.
     * @param cur_message
     * @param buffer
     */
    virtual void
    extract_escaped_string_value_into_buffer(uint64_t cur_message, std::string& buffer) {
        extract_string_value_into_buffer(cur_message, buffer);
    }

private:
    int32_t m_id;
};

class Int64ColumnReader : public BaseColumnReader {
public:
    // Constructor
    explicit Int64ColumnReader(int32_t id) : BaseColumnReader(id) {}

    // Destructor
    ~Int64ColumnReader() override = default;

    // Methods inherited from BaseColumnReader
    void load(BufferViewReader& reader, uint64_t num_messages) override;

    NodeType get_type() override { return NodeType::Integer; }

    std::variant<int64_t, double, std::string, uint8_t> extract_value(uint64_t cur_message
    ) override;

    void extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer) override;

private:
    UnalignedMemSpan<int64_t> m_values;
};

class FloatColumnReader : public BaseColumnReader {
public:
    // Constructor
    explicit FloatColumnReader(int32_t id) : BaseColumnReader(id) {}

    // Destructor
    ~FloatColumnReader() override = default;

    // Methods inherited from BaseColumnReader
    void load(BufferViewReader& reader, uint64_t num_messages) override;

    NodeType get_type() override { return NodeType::Float; }

    std::variant<int64_t, double, std::string, uint8_t> extract_value(uint64_t cur_message
    ) override;

    void extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer) override;

private:
    UnalignedMemSpan<double> m_values;
};

class BooleanColumnReader : public BaseColumnReader {
public:
    // Constructor
    explicit BooleanColumnReader(int32_t id) : BaseColumnReader(id) {}

    // Destructor
    ~BooleanColumnReader() override = default;

    // Methods inherited from BaseColumnReader
    void load(BufferViewReader& reader, uint64_t num_messages) override;

    NodeType get_type() override { return NodeType::Boolean; }

    std::variant<int64_t, double, std::string, uint8_t> extract_value(uint64_t cur_message
    ) override;

    void extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer) override;

private:
    UnalignedMemSpan<uint8_t> m_values;
};

class ClpStringColumnReader : public BaseColumnReader {
public:
    // Constructor
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

    // Destructor
    ~ClpStringColumnReader() override = default;

    // Methods inherited from BaseColumnReader
    void load(BufferViewReader& reader, uint64_t num_messages) override;

    NodeType get_type() override {
        return m_is_array ? NodeType::UnstructuredArray : NodeType::ClpString;
    }

    std::variant<int64_t, double, std::string, uint8_t> extract_value(uint64_t cur_message
    ) override;

    void extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer) override;

    void
    extract_escaped_string_value_into_buffer(uint64_t cur_message, std::string& buffer) override;

    /**
     * Gets the encoded id of the variable
     * @param cur_message
     * @return The encoded logtype id
     */
    int64_t get_encoded_id(uint64_t cur_message);

    /**
     * Gets the encoded variables
     * @param cur_message
     * @return Encoded variables in a span
     */
    UnalignedMemSpan<int64_t> get_encoded_vars(uint64_t cur_message);

private:
    std::shared_ptr<VariableDictionaryReader> m_var_dict;
    std::shared_ptr<LogTypeDictionaryReader> m_log_dict;

    UnalignedMemSpan<uint64_t> m_logtypes;
    UnalignedMemSpan<int64_t> m_encoded_vars;

    bool m_is_array;
};

class VariableStringColumnReader : public BaseColumnReader {
public:
    // Constructor
    VariableStringColumnReader(int32_t id, std::shared_ptr<VariableDictionaryReader> var_dict)
            : BaseColumnReader(id),
              m_var_dict(std::move(var_dict)) {}

    // Destructor
    ~VariableStringColumnReader() override = default;

    // Methods inherited from BaseColumnReader
    void load(BufferViewReader& reader, uint64_t num_messages) override;

    NodeType get_type() override { return NodeType::VarString; }

    std::variant<int64_t, double, std::string, uint8_t> extract_value(uint64_t cur_message
    ) override;

    void extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer) override;

    void
    extract_escaped_string_value_into_buffer(uint64_t cur_message, std::string& buffer) override;

    /**
     * Gets the encoded id of the variable
     * @param cur_message
     * @return The encoded logtype id
     */
    int64_t get_variable_id(uint64_t cur_message);

private:
    std::shared_ptr<VariableDictionaryReader> m_var_dict;

    UnalignedMemSpan<uint64_t> m_variables;
};

class DateStringColumnReader : public BaseColumnReader {
public:
    // Constructor
    DateStringColumnReader(int32_t id, std::shared_ptr<TimestampDictionaryReader> timestamp_dict)
            : BaseColumnReader(id),
              m_timestamp_dict(std::move(timestamp_dict)) {}

    // Destructor
    ~DateStringColumnReader() override = default;

    // Methods inherited from BaseColumnReader
    void load(BufferViewReader& reader, uint64_t num_messages) override;

    NodeType get_type() override { return NodeType::DateString; }

    std::variant<int64_t, double, std::string, uint8_t> extract_value(uint64_t cur_message
    ) override;

    void extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer) override;

    /**
     * @param cur_message
     * @return The encoded time in epoch time
     */
    epochtime_t get_encoded_time(uint64_t cur_message);

private:
    std::shared_ptr<TimestampDictionaryReader> m_timestamp_dict;

    UnalignedMemSpan<int64_t> m_timestamps;
    UnalignedMemSpan<int64_t> m_timestamp_encodings;
};
}  // namespace clp_s

#endif  // CLP_S_COLUMNREADER_HPP
