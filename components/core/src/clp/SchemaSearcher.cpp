#include "SchemaSearcher.hpp"

#include <cstddef>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include <log_surgeon/log_surgeon.hpp>

#include <clp/EncodedVariableInterpreter.hpp>

namespace clp {
using std::holds_alternative;
using std::set;
using std::string;
using std::vector;

auto SchemaSearcher::normalize_interpretations(
        vector<vector<log_surgeon::SubQuery>> const& interpretations
) -> vector<vector<log_surgeon::SubQuery>> {
    vector<vector<log_surgeon::SubQuery>> normalized_interps;
    for (auto const& interpretation : interpretations) {
        vector<log_surgeon::SubQuery> normalized_interp(interpretation.size());
        for (size_t i{0}; i < interpretation.size(); ++i) {
            auto const& token{interpretation[i]};
            auto& normalized_token{normalized_interp[i]};
            normalized_token.qualified_name = token.qualified_name;

            auto& normalized_value{normalized_token.value};
            normalized_value.reserve(token.value.length());
            for (auto const c : token.value) {
                if ('*' != c || normalized_value.empty() || '*' != normalized_value.back()) {
                    normalized_value += c;
                }
            }
        }
        if (normalized_interps.end() == std::ranges::find(normalized_interps, normalized_interp)) {
            normalized_interps.push_back(std::move(normalized_interp));
        }
    }
    return normalized_interps;
}

auto SchemaSearcher::get_wildcard_encodable_positions(
        vector<log_surgeon::SubQuery> const& interpretation
) -> vector<size_t> {
    vector<size_t> wildcard_encodable_positions;
    wildcard_encodable_positions.reserve(interpretation.size());

    for (size_t i{0}; i < interpretation.size(); ++i) {
        auto const& token{interpretation[i]};
        if (false == token.qualified_name.empty()) {
            auto const var_type{token.qualified_name};
            bool const is_int{var_type.ends_with("int")};
            bool const is_float{var_type.ends_with("float")};
            bool contains_wildcard{false};
            for (size_t j{0}; j < token.value.size(); ++j) {
                if ('*' == token.value[j] || '?' == token.value[j]) {
                    if (0 == j || '\\' != token.value[j - 1]) {
                        contains_wildcard = true;
                        break;
                    }
                }
            }
            if (contains_wildcard && (is_int || is_float)) {
                wildcard_encodable_positions.push_back(i);
            }
        }
    }
    return wildcard_encodable_positions;
}

auto SchemaSearcher::generate_logtype_string(
        vector<log_surgeon::SubQuery> const& interpretation,
        vector<size_t> const& wildcard_encodable_positions,
        vector<bool> const& mask_encoded_flags
) -> string {
    string logtype_string;

    size_t logtype_string_size{0};
    for (auto const& token : interpretation) {
        if (token.qualified_name.empty()) {
            logtype_string_size += token.value.size();
        } else {
            ++logtype_string_size;
        }
    }
    logtype_string.reserve(logtype_string_size);

    for (size_t i{0}; i < interpretation.size(); ++i) {
        auto const& token{interpretation[i]};
        if (token.qualified_name.empty()) {
            logtype_string += token.value;
            continue;
        }

        auto const var_type{token.qualified_name};
        bool const is_int{var_type.ends_with("int")};
        bool const is_float{var_type.ends_with("float")};

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
                    token.value,
                    encoded_var
            ))
        {
            EncodedVariableInterpreter::add_int_var(logtype_string);
        } else if (is_float
                   && EncodedVariableInterpreter::convert_string_to_representable_float_var(
                           token.value,
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
