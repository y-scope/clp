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

    std::variant<int64_t, double, std::string, uint8_t> extract_value(
            uint64_t cur_message
    ) override;

    void extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer) override;

private:
    UnalignedMemSpan<int64_t> m_values;
};

class DeltaEncodedInt64ColumnReader : public BaseColumnReader {
public:
    // Constructor
    explicit DeltaEncodedInt64ColumnReader(int32_t id) : BaseColumnReader(id) {}

    // Destructor
    ~DeltaEncodedInt64ColumnReader() override = default;

    // Methods inherited from BaseColumnReader
    void load(BufferViewReader& reader, uint64_t num_messages) override;

    NodeType get_type() override { return NodeType::DeltaInteger; }

    std::variant<int64_t, double, std::string, uint8_t> extract_value(
            uint64_t cur_message
    ) override;

    void extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer) override;

private:
    /**
     * Gets the value stored at a given index by summing up the stored deltas between the requested
     * index and the last requested index.
     * @param idx
     * @return The value stored at the requested index.
     */
    int64_t get_value_at_idx(size_t idx);

    UnalignedMemSpan<int64_t> m_values;
    int64_t m_cur_value{};
    size_t m_cur_idx{};
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

    std::variant<int64_t, double, std::string, uint8_t> extract_value(
            uint64_t cur_message
    ) override;

    void extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer) override;

private:
    UnalignedMemSpan<double> m_values;
};

class FormattedFloatColumnReader : public BaseColumnReader {
public:
    // Constructor
    explicit FormattedFloatColumnReader(int32_t id) : BaseColumnReader(id) {}

    // Destructor
    ~FormattedFloatColumnReader() override = default;

    // Methods inherited from BaseColumnReader
    void load(BufferViewReader& reader, uint64_t num_messages) override;

    NodeType get_type() override { return NodeType::FormattedFloat; }

    std::variant<int64_t, double, std::string, uint8_t> extract_value(
            uint64_t cur_message
    ) override;

    void extract_string_value_into_buffer(uint64_t cur_message, std::string& buffer) override;

    /**
     * Decodes the format information and generates the string of the stored double value with the
     * decoded format.
     *
     * @param cur_message
     * @return The double value string formatted by the format information.
     */
    std::string restore_format(uint64_t cur_message) const;

private:
    static constexpr uint16_t cEmptyExponentSign = 0b00;
    static constexpr uint16_t cPlusExponentSign = 0b01;
    static constexpr uint16_t cMinusExponentSign = 0b10;

    UnalignedMemSpan<double> m_values;
    UnalignedMemSpan<uint16_t> m_format;

    bool has_exponent_sign(uint64_t cur_message, uint16_t sign) const;
    bool has_scientific_notation(uint64_t cur_message) const;
    bool is_uppercase_exponent(uint64_t cur_message) const;

    uint16_t get_exponent_digits(uint64_t cur_message) const;
    uint16_t get_significant_digits(uint64_t cur_message) const;

    /**
     * Trims the leading zeros until the number of exponent digits match the value stored in the
     * format. The function attempts to remove up to `number_of_zeros_to_trim` leading zeros from
     * the exponent, but stops early if a non-zero digit is encountered to preserve correctness.
     *
     * @param scientific_notation The scientific notation string generated by std::scientific.
     * @param start The start position of trimming. It could be either right after the exponent
     * note (E or e), or the second char after the exponent note if the exponent has a sign.
     * @param number_of_zeros_to_trim The difference between current number of exponent digits and
     * the target value stored in the format.
     * @return The scientific notation string with the exponent being trimmed leading zeros.
     */
    static std::string trim_leading_zeros(
            std::string_view scientific_notation,
            size_t start,
            size_t number_of_zeros_to_trim
    );

    /**
     * Convert the scientific notation string to a double value string formatted by the encoded
     * format information.
     *
     * @param scientific_notation The scientific notation string generated by std::scientific.
     * @return The double value string formatted by the format information.
     */
    static std::string scientific_to_decimal(std::string_view scientific_notation);
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

    std::variant<int64_t, double, std::string, uint8_t> extract_value(
            uint64_t cur_message
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

    std::variant<int64_t, double, std::string, uint8_t> extract_value(
            uint64_t cur_message
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

    std::variant<int64_t, double, std::string, uint8_t> extract_value(
            uint64_t cur_message
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

    std::variant<int64_t, double, std::string, uint8_t> extract_value(
            uint64_t cur_message
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
