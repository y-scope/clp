#include "SchemaSearcher.hpp"

#include <cstddef>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include <log_surgeon/Constants.hpp>

#include <clp/EncodedVariableInterpreter.hpp>

using log_surgeon::SymbolId::TokenFloat;
using log_surgeon::SymbolId::TokenInt;
using log_surgeon::wildcard_query_parser::QueryInterpretation;
using log_surgeon::wildcard_query_parser::StaticQueryToken;
using log_surgeon::wildcard_query_parser::VariableQueryToken;
using std::holds_alternative;
using std::set;
using std::string;
using std::vector;

namespace clp {
auto SchemaSearcher::normalize_interpretations(set<QueryInterpretation> const& interpretations)
        -> set<QueryInterpretation> {
    set<QueryInterpretation> normalized_interpretations;
    for (auto const& interpretation : interpretations) {
        QueryInterpretation normalized_interpretation;
        for (auto const& token : interpretation.get_logtype()) {
            auto const& src_string{std::visit(
                    [](auto const& tok) -> string const& { return tok.get_query_substring(); },
                    token
            )};
            string normalized_string;
            normalized_string.reserve(src_string.size());
            for (auto const c : src_string) {
                if ('*' != c || normalized_string.empty() || '*' != normalized_string.back()) {
                    normalized_string += c;
                }
            }

            std::visit(
                    overloaded{
                            [&](VariableQueryToken const& variable_token) -> void {
                                normalized_interpretation.append_variable_token(
                                        variable_token.get_variable_type(),
                                        normalized_string,
                                        variable_token.get_contains_wildcard()
                                );
                            },
                            [&]([[maybe_unused]] StaticQueryToken const& static_token) -> void {
                                normalized_interpretation.append_static_token(normalized_string);
                            }
                    },
                    token
            );
        }
        normalized_interpretations.insert(normalized_interpretation);
    }
    return normalized_interpretations;
}

auto SchemaSearcher::get_wildcard_encodable_positions(QueryInterpretation const& interpretation)
        -> vector<size_t> {
    auto const logtype{interpretation.get_logtype()};
    vector<size_t> wildcard_encodable_positions;
    wildcard_encodable_positions.reserve(logtype.size());

    for (size_t i{0}; i < logtype.size(); ++i) {
        auto const& token{logtype[i]};
        if (holds_alternative<VariableQueryToken>(token)) {
            auto const& var_token{std::get<VariableQueryToken>(token)};
            auto const var_type{static_cast<log_surgeon::SymbolId>(var_token.get_variable_type())};
            bool const is_int{TokenInt == var_type};
            bool const is_float{TokenFloat == var_type};
            if (var_token.get_contains_wildcard() && (is_int || is_float)) {
                wildcard_encodable_positions.push_back(i);
            }
        }
    }
    return wildcard_encodable_positions;
}

auto SchemaSearcher::generate_logtype_string(
        QueryInterpretation const& interpretation,
        vector<size_t> const& wildcard_encodable_positions,
        vector<bool> const& mask_encoded_flags
) -> string {
    string logtype_string;

    size_t logtype_string_size{0};
    auto const logtype{interpretation.get_logtype()};
    for (auto const& token : logtype) {
        if (holds_alternative<StaticQueryToken>(token)) {
            auto const& static_token{std::get<StaticQueryToken>(token)};
            logtype_string_size += static_token.get_query_substring().size();
        } else {
            logtype_string_size++;
        }
    }
    logtype_string.reserve(logtype_string_size);

    for (size_t i{0}; i < logtype.size(); ++i) {
        auto const& token{logtype[i]};
        if (holds_alternative<StaticQueryToken>(token)) {
            logtype_string += std::get<StaticQueryToken>(token).get_query_substring();
            continue;
        }

        auto const& var_token{std::get<VariableQueryToken>(token)};
        auto const& raw_string{var_token.get_query_substring()};
        auto const var_type{static_cast<log_surgeon::SymbolId>(var_token.get_variable_type())};
        bool const is_int{TokenInt == var_type};
        bool const is_float{TokenFloat == var_type};

        if (wildcard_encodable_positions.end()
            != std::ranges::find(wildcard_encodable_positions, i))
        {
            if (mask_encoded_flags[i]) {
                if (is_int) {
                    EncodedVariableInterpreter::add_int_var(logtype_string);
                } else {
                    EncodedVariableInterpreter::add_float_var(logtype_string);
                }
            } else {
                EncodedVariableInterpreter::add_dict_var(logtype_string);
            }
            continue;
        }

        encoded_variable_t encoded_var{0};
        if (is_int
            && EncodedVariableInterpreter::convert_string_to_representable_integer_var(
                    raw_string,
                    encoded_var
            ))
        {
            EncodedVariableInterpreter::add_int_var(logtype_string);
        } else if (is_float
                   && EncodedVariableInterpreter::convert_string_to_representable_float_var(
                           raw_string,
                           encoded_var
                   ))
        {
            EncodedVariableInterpreter::add_float_var(logtype_string);
        } else {
            EncodedVariableInterpreter::add_dict_var(logtype_string);
        }
    }
    return logtype_string;
}
}  // namespace clp
