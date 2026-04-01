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
/**
 * Used to analzye a schema and determine which variables/captures are encodable. A variable is
 * encodable if there exists any string it can match that also matches an encoded type.
 *
 * The following is how it is used:
 * - Set the delimiters to match those used in the schema using `set_delimiters`.
 * - Add each encoding type using `add_encoding_type`.
 * - Generate the analyzer using `generate`.
 * - Generate the map of mathes using `identify_encoded_vars_in_schema`.
 * - Get the map of matches using `get_map`.
 */
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
    using CaptureMap = std::unordered_map<std::string, std::vector<std::unique_ptr<RegexAST>>>;
    using EncodingMap = std::unordered_map<std::string, std::unordered_set<std::string>>;

    /**
     * Sets the delimiters to be used when replacing wildcards.
     *
     * @param delimiters The delimiters to be set.
     */
    auto set_delimiters(std::vector<uint32_t> delimiters) -> void;

    /**
     * Add an encoding to test for in the schema. Must be added before calling `generate`.
     *
     * @param var_name The name of the encoded type.
     * @param var_regex The regex of the encoded type.
     */
    auto add_encoding_type(std::string const& var_name, std::string const& var_regex) -> void;

    /**
     * Build the schema analyzer using the specified encoded types.
     */
    auto generate() -> void;

    /**
     * Compare all variables and captures in the schema against the encoded type. Stores the results
     * in `m_encoded_type_to_schema_vars`
     *
     * @param schema The schema to analyze.
     */
    auto identify_encoded_vars_in_schema(std::unique_ptr<log_surgeon::SchemaAST> schema) -> void;

    auto get_map() -> EncodingMap const& {
        return m_encoded_type_to_schema_vars;
    }

private:
    /**
     * Compares a single variable/capture against the encoded DFA using an
     * intersection and returns the types the variable/capture is encodable as.
     *
     * @param regex_ast
     * @return
     */
    [[nodiscard]] auto get_encoded_types(std::unique_ptr<RegexAST> regex_ast) -> std::set<uint32_t>;

    /**
     * Traverses the AST to find all captures.
     *
     * @param user_schema_ast The AST to traverse.
     * @return A map of capture names to capture regex.
     */
    auto get_captures(std::unique_ptr<log_surgeon::SchemaAST> const& user_schema_ast) -> CaptureMap;

    /**
     * A helper for traversing the AST to find all captures.
     *
     * @param ast The sub-AST to traverse.
     * @param capture_map A map of capture names to capture regex.
     */
    auto recurse_captures(RegexAST const* ast, CaptureMap& capture_map) -> void;

    std::vector<uint32_t> m_delimiters;
    std::unordered_map<uint32_t, std::string> m_names;
    std::unordered_map<std::string, uint32_t> m_symbols;
    std::vector<log_surgeon::LexicalRule<ByteNfaState>> m_rules;
    std::unique_ptr<ByteDfa> m_encoded_dfa;
    EncodingMap m_encoded_type_to_schema_vars;
};
}  // namespace clp::clp

#endif  // CLP_CLP_SCHEMANALYZER_HPP
