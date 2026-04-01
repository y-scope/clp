#include "SchemaAnalyzer.hpp"

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <log_surgeon/finite_automata/RegexAST.hpp>
#include <log_surgeon/LexicalRule.hpp>
#include <log_surgeon/Schema.hpp>
#include <log_surgeon/SchemaParser.hpp>

namespace clp::clp {
using log_surgeon::finite_automata::ByteNfaState;
using log_surgeon::LexicalRule;
using log_surgeon::Schema;
using log_surgeon::SchemaAST;
using log_surgeon::SchemaVarAST;
using std::set;
using std::string;
using std::vector;

using RegexASTCapture = log_surgeon::finite_automata::RegexASTCapture<ByteNfaState>;
using RegexASTCat = log_surgeon::finite_automata::RegexASTCat<ByteNfaState>;
using RegexASTMultiplication = log_surgeon::finite_automata::RegexASTMultiplication<ByteNfaState>;
using RegexASTOr = log_surgeon::finite_automata::RegexASTOr<ByteNfaState>;

auto SchemaAnalyzer::set_delimiters(vector<uint32_t> delimiters) -> void {
    m_delimiters = std::move(delimiters);
}

auto SchemaAnalyzer::add_encoding_type(string const& var_name, string const& var_regex) -> void {
    if (false == m_symbols.contains(var_name)) {
        m_symbols.emplace(var_name, m_symbols.size());
        m_names.emplace(m_names.size(), var_name);
    }

    Schema schema;
    schema.add_variable(var_name + ":" + var_regex, -1);
    auto const schema_ast{schema.release_schema_ast_ptr()};
    auto* var{dynamic_cast<SchemaVarAST*>(schema_ast->m_schema_vars[0].get())};
    var->m_regex_ptr->remove_delimiters_from_wildcard(m_delimiters);
    m_rules.emplace_back(m_symbols[var_name], std::move(var->m_regex_ptr));
}

auto SchemaAnalyzer::generate() -> void {
    ByteNfa const nfa{m_rules};
    m_encoded_dfa = std::make_unique<ByteDfa>(nfa);
}

auto SchemaAnalyzer::identify_encoded_vars_in_schema(std::unique_ptr<SchemaAST> schema) -> void {
    // The following can be simplified if `m_rules` in the lexer was accessible
    auto capture_map{get_captures(schema)};
    for (auto const& parser_ast : schema->m_schema_vars) {
        auto* variable{dynamic_cast<SchemaVarAST*>(parser_ast.get())};
        auto encoded_types{get_encoded_types(std::move(variable->m_regex_ptr))};
        for (auto const encoded_type : encoded_types) {
            auto const& encoded_name{m_names.at(encoded_type)};
            m_encoded_type_to_schema_vars[encoded_name].insert(variable->m_name);
        }
    }
    for (auto& [capture_name, capture_regexes] : capture_map) {
        for (auto& capture_regex : capture_regexes) {
            auto encoded_types{get_encoded_types(std::move(capture_regex))};
            for (auto const encoded_type : encoded_types) {
                auto const& encoded_name{m_names.at(encoded_type)};
                m_encoded_type_to_schema_vars[encoded_name].insert(capture_name);
            }
        }
    }
}

auto SchemaAnalyzer::get_encoded_types(std::unique_ptr<RegexAST> regex_ast) -> set<uint32_t> {
    regex_ast->remove_delimiters_from_wildcard(m_delimiters);
    std::vector<LexicalRule<ByteNfaState>> rules;
    rules.emplace_back(0, std::move(regex_ast));
    ByteNfa const nfa(rules);
    ByteDfa dfa(nfa);
    return m_encoded_dfa->get_intersect(&dfa);
}

auto SchemaAnalyzer::get_captures(std::unique_ptr<SchemaAST> const& user_schema_ast) -> CaptureMap {
    CaptureMap capture_map;
    for (auto const& parser_ast : user_schema_ast->m_schema_vars) {
        auto* variable{dynamic_cast<SchemaVarAST*>(parser_ast.get())};
        recurse_captures(variable->m_regex_ptr.get(), capture_map);
    }
    return capture_map;
}

auto SchemaAnalyzer::recurse_captures(RegexAST const* ast, CaptureMap& capture_map) -> void {
    if (auto const* alt{dynamic_cast<RegexASTOr const*>(ast)}) {
        recurse_captures(alt->get_left(), capture_map);
        recurse_captures(alt->get_right(), capture_map);
        return;
    }
    if (auto const* cat{dynamic_cast<RegexASTCat const*>(ast)}) {
        recurse_captures(cat->get_left(), capture_map);
        recurse_captures(cat->get_right(), capture_map);
        return;
    }
    if (auto const* multi{dynamic_cast<RegexASTMultiplication const*>(ast)}) {
        recurse_captures(multi->get_operand().get(), capture_map);
        return;
    }
    if (auto const* cap{dynamic_cast<RegexASTCapture const*>(ast)}) {
        recurse_captures(cap->get_capture_regex_ast().get(), capture_map);
        capture_map[string(cap->get_capture_name())].emplace_back(cap->clone());
    }
}
}  // namespace clp::clp
