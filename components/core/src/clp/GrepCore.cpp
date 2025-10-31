#include "GrepCore.hpp"

#include <algorithm>
#include <cstddef>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include <log_surgeon/Constants.hpp>
#include <string_utils/string_utils.hpp>

#include "EncodedVariableInterpreter.hpp"
#include "ir/parsing.hpp"
#include "StringReader.hpp"

using clp::ir::is_delim;
using clp::string_utils::is_alphabet;
using clp::string_utils::is_wildcard;
using log_surgeon::SymbolId::TokenFloat;
using log_surgeon::SymbolId::TokenInt;
using log_surgeon::wildcard_query_parser::QueryInterpretation;
using log_surgeon::wildcard_query_parser::StaticQueryToken;
using log_surgeon::wildcard_query_parser::VariableQueryToken;
using std::holds_alternative;
using std::set;
using std::string;
using std::unordered_map;
using std::vector;

namespace clp {
bool GrepCore::get_bounds_of_next_potential_var(
        string const& value,
        size_t& begin_pos,
        size_t& end_pos,
        bool& is_var
) {
    auto const value_length = value.length();
    if (end_pos >= value_length) {
        return false;
    }

    is_var = false;
    bool contains_wildcard = false;
    while (false == is_var && false == contains_wildcard && begin_pos < value_length) {
        // Start search at end of last token
        begin_pos = end_pos;

        // Find next wildcard or non-delimiter
        bool is_escaped = false;
        for (; begin_pos < value_length; ++begin_pos) {
            char c = value[begin_pos];

            if (is_escaped) {
                is_escaped = false;

                if (false == is_delim(c)) {
                    // Found escaped non-delimiter, so reverse the index to retain the escape
                    // character
                    --begin_pos;
                    break;
                }
            } else if ('\\' == c) {
                // Escape character
                is_escaped = true;
            } else {
                if (is_wildcard(c)) {
                    contains_wildcard = true;
                    break;
                }
                if (false == is_delim(c)) {
                    break;
                }
            }
        }

        bool contains_decimal_digit = false;
        bool contains_alphabet = false;

        // Find next delimiter
        is_escaped = false;
        end_pos = begin_pos;
        for (; end_pos < value_length; ++end_pos) {
            char c = value[end_pos];

            if (is_escaped) {
                is_escaped = false;

                if (is_delim(c)) {
                    // Found escaped delimiter, so reverse the index to retain the escape character
                    --end_pos;
                    break;
                }
            } else if ('\\' == c) {
                // Escape character
                is_escaped = true;
            } else {
                if (is_wildcard(c)) {
                    contains_wildcard = true;
                } else if (is_delim(c)) {
                    // Found delimiter that's not also a wildcard
                    break;
                }
            }

            if (string_utils::is_decimal_digit(c)) {
                contains_decimal_digit = true;
            } else if (is_alphabet(c)) {
                contains_alphabet = true;
            }
        }

        // Treat token as a definite variable if:
        // - it contains a decimal digit, or
        // - it could be a multi-digit hex value, or
        // - it's directly preceded by an equals sign and contains an alphabet without a wildcard
        //   between the equals sign and the first alphabet of the token
        auto variable = static_cast<std::string_view>(value).substr(begin_pos, end_pos - begin_pos);
        if (contains_decimal_digit || ir::could_be_multi_digit_hex_value(variable)) {
            is_var = true;
        } else if (begin_pos > 0 && '=' == value[begin_pos - 1] && contains_alphabet) {
            // Find first alphabet or wildcard in token
            is_escaped = false;
            bool found_wildcard_before_alphabet = false;
            for (auto i = begin_pos; i < end_pos; ++i) {
                auto c = value[i];

                if (is_escaped) {
                    is_escaped = false;

                    if (is_alphabet(c)) {
                        break;
                    }
                } else if ('\\' == c) {
                    // Escape character
                    is_escaped = true;
                } else if (is_wildcard(c)) {
                    found_wildcard_before_alphabet = true;
                    break;
                }
            }

            if (false == found_wildcard_before_alphabet) {
                is_var = true;
            }
        }
    }

    return (value_length != begin_pos);
}

auto GrepCore::normalize_interpretations(set<QueryInterpretation> const& interpretations)
        -> set<QueryInterpretation> {
    set<QueryInterpretation> normalized_interpretations;
    for (auto const& interpretation : interpretations) {
        QueryInterpretation normalized_interpretation;
        for (auto const& token : interpretation.get_logtype()) {
            auto const& src_string{
                    holds_alternative<VariableQueryToken>(token)
                            ? std::get<VariableQueryToken>(token).get_query_substring()
                            : std::get<StaticQueryToken>(token).get_query_substring()
            };
            string normalized_string;
            normalized_string.reserve(src_string.size());
            for (auto const c : src_string) {
                if (c != '*' || normalized_string.empty() || normalized_string.back() != '*') {
                    normalized_string += c;
                }
            }

            if (holds_alternative<VariableQueryToken>(token)) {
                auto const& variable_token{std::get<VariableQueryToken>(token)};
                normalized_interpretation.append_variable_token(
                        variable_token.get_variable_type(),
                        normalized_string,
                        variable_token.get_contains_wildcard()
                );
            } else {
                normalized_interpretation.append_static_token(normalized_string);
            }
        }
        normalized_interpretations.insert(normalized_interpretation);
    }
    return normalized_interpretations;
}

auto GrepCore::get_wildcard_encodable_positions(QueryInterpretation const& interpretation)
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

auto GrepCore::generate_logtype_string(
        QueryInterpretation const& interpretation,
        vector<size_t> const& wildcard_encodable_positions,
        vector<bool> const& mask_encoded_flags
) -> string {
    string logtype_string;

    // Reserve size for `logtype_string`.
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

    // Generate `logtype_string`.
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
            != std::ranges::find(
                    wildcard_encodable_positions.begin(),
                    wildcard_encodable_positions.end(),
                    i
            ))
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
