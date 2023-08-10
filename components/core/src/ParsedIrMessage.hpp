#ifndef ParsedIrMessage_HPP
#define ParsedIrMessage_HPP

// C++ standard libraries
#include <cassert>
#include <string>
#include <vector>

// Project headers
#include "Defs.h"
#include "LogTypeDictionaryEntry.hpp"
#include "TimestampPattern.hpp"

/**
 * ParsedIRMessage represents a (potentially multiline) log message parsed from encoded ir
 * into four primary fields: logtype_entry, variables, timestamp and timestamp pattern.
 */
class ParsedIrMessage {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) :
                  TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "ParsedIrMessage operation failed";
        }
    };

    enum class VariableType {
        EncodedVar = 0,
        DictVar,
        Length
    };

    // Helper class to keep variables in the order as they appear in the
    // original log messages
    class IrVariable {
    public:
        IrVariable(const std::string& dict_var) {
            m_dict_var = dict_var;
            m_type = VariableType::DictVar;
        }
        IrVariable(encoded_variable_t encoded_var) {
            m_encoded_var = encoded_var;
            m_type = VariableType::EncodedVar;
        }

        // Methods
        VariableType type() const {
            return m_type;
        }

        encoded_variable_t get_encoded_var () const {
            assert(m_type == VariableType::EncodedVar);
            return m_encoded_var;
        }

        const std::string& get_dict_var () const {
            assert(m_type == VariableType::DictVar);
            return m_dict_var;
        }

    private:
        std::string m_dict_var;
        encoded_variable_t m_encoded_var;
        VariableType m_type;
    };

    // Construtor
    ParsedIrMessage() : m_ts_patt(nullptr) {}

    // Methods
    void clear();
    void clear_except_ts_patt ();

    // setter
    void set_ts (epochtime_t ts);
    void set_ts_pattern (const TimestampPattern* timestamp_pattern);

    // note, this logtype is already escaped
    void append_to_logtype (const std::string& value, size_t begin_pos, size_t length);
    void add_encoded_integer (encoded_variable_t var, size_t original_size_in_bytes);
    void add_encoded_float (encoded_variable_t var, size_t original_size_in_bytes);
    void add_dictionary_var (const std::string& dictionary_var);

    // getter
    epochtime_t get_ts () const { return m_ts; }
    LogTypeDictionaryEntry& get_logtype_entry () { return m_logtype_entry; }
    const std::vector<IrVariable>& get_vars () const { return m_variables; }
    size_t get_orig_num_bytes() const { return m_orig_num_bytes; }

private:
    // Variables
    const TimestampPattern* m_ts_patt;
    epochtime_t m_ts;
    LogTypeDictionaryEntry m_logtype_entry;
    std::vector<IrVariable> m_variables;
    size_t m_orig_num_bytes;
    size_t m_ts_bytes;
};


#endif // ParsedIrMessage_HPP