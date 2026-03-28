#ifndef CLP_CLP_SCHEMANALYZER_HPP
#define CLP_CLP_SCHEMANALYZER_HPP

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <log_surgeon/finite_automata/Capture.hpp>
#include <log_surgeon/finite_automata/Dfa.hpp>
#include <log_surgeon/finite_automata/DfaState.hpp>
#include <log_surgeon/finite_automata/Nfa.hpp>
#include <log_surgeon/finite_automata/NfaState.hpp>
#include <log_surgeon/finite_automata/RegexAST.hpp>
#include <log_surgeon/LexicalRule.hpp>
#include <log_surgeon/SchemaParser.hpp>

namespace clp::clp {
class SchemaAnalyzer {
public:
    using ByteDfaState = log_surgeon::finite_automata::ByteDfaState;
    using ByteNfaState = log_surgeon::finite_automata::ByteNfaState;
    using ByteDfa = log_surgeon::finite_automata::Dfa<ByteDfaState, ByteNfaState>;
    using ByteNfa = log_surgeon::finite_automata::Nfa<ByteNfaState>;
    using Capture = log_surgeon::finite_automata::Capture;
    using RegexAST = log_surgeon::finite_automata::RegexAST<ByteNfaState>;
    // CaptureMap should use a `Capture const*` key, but its inaccessible in `CaptureAST` currently.
    // This means if any capture with the name is encodable, all are treated as encodable.
    using CaptureMap = std::unordered_map<std::string, std::unique_ptr<RegexAST>>;

    auto set_delimiters(std::vector<uint32_t> delimiters) -> void;

    auto add_encoded_var(std::string const& var_name, std::string const& var_regex) -> void;

    auto generate() -> void;

    auto identify_encoded_vars_in_schema(std::unique_ptr<log_surgeon::SchemaAST> schema) -> void;

    auto get_map() -> std::unordered_map<std::string, std::unordered_set<std::string>> const& {
        return m_encoded_type_to_schema_vars;
    }

private:
    [[nodiscard]] auto get_encoded_types(std::unique_ptr<RegexAST> regex_ast) -> std::set<uint32_t>;

    auto get_captures(std::unique_ptr<log_surgeon::SchemaAST> const& user_schema_ast) -> CaptureMap;

    auto recurse_captures(RegexAST const* ast, CaptureMap& capture_map) -> void;

    std::vector<uint32_t> m_delimiters;
    std::unordered_map<uint32_t, std::string> m_names;
    std::unordered_map<std::string, uint32_t> m_symbols;
    std::vector<log_surgeon::LexicalRule<ByteNfaState>> m_rules;
    std::unique_ptr<ByteDfa> m_encoded_dfa;
    std::unordered_map<std::string, std::unordered_set<std::string>> m_encoded_type_to_schema_vars;
};
}  // namespace clp::clp

#endif  // CLP_CLP_SCHEMANALYZER_HPP
