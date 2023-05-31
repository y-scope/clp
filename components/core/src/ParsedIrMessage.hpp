#ifndef ParsedIrMessage_HPP
#define ParsedIrMessage_HPP

// C++ standard libraries
#include <cassert>
#include <string>
#include <vector>

// Project headers
#include "Defs.h"
#include "TimestampPattern.hpp"

class ParsedIrMessage {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

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
    class EncodedIrVariable {
    public:
        EncodedIrVariable(const std::string& dict_var) {
            m_dict_var = dict_var;
            m_type = VariableType::DictVar;
        }
        EncodedIrVariable(encoded_variable_t encoded_var) {
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
    const std::string& get_logtype () const { return m_logtype; }
    const std::vector<EncodedIrVariable>& get_vars () const { return m_variables; }
    const std::vector<size_t>& get_var_positions () const { return m_var_positions; }
    size_t get_orig_num_bytes() const { return m_orig_num_bytes; }
private:

    // Variables
    const TimestampPattern* m_ts_patt;
    epochtime_t m_ts;
    std::string m_logtype;
    std::vector<EncodedIrVariable> m_variables;
    std::vector<size_t> m_var_positions;
    size_t m_orig_num_bytes;
};


#endif // ParsedIrMessage_HPP
