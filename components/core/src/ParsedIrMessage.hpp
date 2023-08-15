#ifndef PARSEDIRMESSAGE_HPP
#define PARSEDIRMESSAGE_HPP

#include <cassert>
#include <string>
#include <utility>
#include <vector>

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
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        [[nodiscard]] auto what() const noexcept -> char const* override {
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
        explicit IrVariable(std::string dict_var)
                : m_dict_var(std::move(dict_var)),
                  m_type(VariableType::DictVar) {}

        explicit IrVariable(encoded_variable_t encoded_var)
                : m_encoded_var(encoded_var),
                  m_type(VariableType::EncodedVar) {}

        // Methods
        [[nodiscard]] auto type() const -> VariableType { return m_type; }

        [[nodiscard]] auto get_encoded_var() const -> encoded_variable_t {
            assert(m_type == VariableType::EncodedVar);
            return m_encoded_var;
        }

        [[nodiscard]] auto get_dict_var() const -> std::string const& {
            assert(m_type == VariableType::DictVar);
            return m_dict_var;
        }

    private:
        std::string m_dict_var;
        encoded_variable_t m_encoded_var{0};
        VariableType m_type;
    };

    // Methods
    auto clear() -> void;
    auto clear_except_ts_patt() -> void;

    // setter
    auto set_ts(epochtime_t ts) -> void;
    auto set_ts_pattern(TimestampPattern const* timestamp_pattern) -> void;

    // note, this logtype is already escaped
    auto append_to_logtype(std::string const& value, size_t begin_pos, size_t length) -> void;
    auto add_encoded_integer(encoded_variable_t var, size_t original_size_in_bytes) -> void;
    auto add_encoded_float(encoded_variable_t var, size_t original_size_in_bytes) -> void;
    auto add_dictionary_var(std::string const& dictionary_var) -> void;

    // getter
    [[nodiscard]] auto get_ts() const -> epochtime_t { return m_ts; }

    auto get_logtype_entry() -> LogTypeDictionaryEntry& { return m_logtype_entry; }

    [[nodiscard]] auto get_vars() const -> std::vector<IrVariable> const& { return m_variables; }

    [[nodiscard]] auto get_orig_num_bytes() const -> size_t { return m_orig_num_bytes; }

private:
    // Variables
    TimestampPattern const* m_ts_patt{nullptr};
    epochtime_t m_ts{0};
    LogTypeDictionaryEntry m_logtype_entry;
    std::vector<IrVariable> m_variables;
    size_t m_orig_num_bytes{0};
    size_t m_ts_bytes{0};
};

#endif  // PARSEDIRMESSAGE_HPP
