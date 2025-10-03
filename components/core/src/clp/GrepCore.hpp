#ifndef CLP_GREPCORE_HPP
#define CLP_GREPCORE_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>
#include <vector>

#include <log_surgeon/Lexer.hpp>
#include <log_surgeon/wildcard_query_parser/Query.hpp>
#include <string_utils/constants.hpp>
#include <string_utils/string_utils.hpp>

#include "Defs.h"
#include "EncodedVariableInterpreter.hpp"
#include "ir/parsing.hpp"
#include "ir/types.hpp"
#include "LogTypeDictionaryReaderReq.hpp"
#include "Query.hpp"
#include "QueryToken.hpp"
#include "VariableDictionaryReaderReq.hpp"

namespace clp {
#ifdef CLP_BUILD_TESTING
class GrepCoreTest;
#endif

class GrepCore {
#ifdef CLP_BUILD_TESTING
    friend class GrepCoreTest;
#endif

public:
    // Methods
    /**
     * Processes a raw user query into a Query.
     *
     * NOTE: Callers are responsible for ensuring that the search string is "clean", where clean is
     * defined by the criteria in `clean_up_wildcard_search_string`.
     *
     * @tparam LogTypeDictionaryReaderType
     * @tparam VariableDictionaryReaderType
     * @param logtype_dict
     * @param var_dict
     * @param search_string
     * @param search_begin_ts
     * @param search_end_ts
     * @param ignore_case
     * @param lexer DFA for determining if input is in the schema
     * @param use_heuristic
     * @return Query if it may match a message, std::nullopt otherwise
     */
    template <
            LogTypeDictionaryReaderReq LogTypeDictionaryReaderType,
            VariableDictionaryReaderReq VariableDictionaryReaderType
    >
    static std::optional<Query> process_raw_query(
            LogTypeDictionaryReaderType const& logtype_dict,
            VariableDictionaryReaderType const& var_dict,
            std::string const& search_string,
            epochtime_t search_begin_ts,
            epochtime_t search_end_ts,
            bool ignore_case,
            log_surgeon::lexers::ByteLexer& lexer,
            bool use_heuristic
    );

    /**
     * Returns bounds of next potential variable (either a definite variable or a token with
     * wildcards)
     * @param value String containing token
     * @param begin_pos Begin position of last token, changes to begin position of next token
     * @param end_pos End position of last token, changes to end position of next token
     * @param is_var Whether the token is definitely a variable
     * @return true if another potential variable was found, false otherwise
     */
    static bool get_bounds_of_next_potential_var(
            std::string const& value,
            size_t& begin_pos,
            size_t& end_pos,
            bool& is_var
    );

private:
    // Types
    enum class SubQueryMatchabilityResult : uint8_t {
        MayMatch,  // The subquery might match a message
        WontMatch,  // The subquery has no chance of matching a message
        SupercedesAllSubQueries  // The subquery will cause all messages to be matched
    };

    // Methods
    /**
     * Process a QueryToken that is definitely a variable.
     * @tparam VariableDictionaryReaderType
     * @param query_token
     * @param var_dict
     * @param ignore_case
     * @param sub_query
     * @param logtype
     * @return true if this token might match a message, false otherwise
     */
    template <VariableDictionaryReaderReq VariableDictionaryReaderType>
    static bool process_var_token(
            QueryToken const& query_token,
            VariableDictionaryReaderType const& var_dict,
            bool ignore_case,
            SubQuery& sub_query,
            std::string& logtype
    );

    /**
     * Generates logtypes and variables for subquery.
     * @tparam LogTypeDictionaryReaderType
     * @tparam VariableDictionaryReaderType
     * @param logtype_dict
     * @param var_dict
     * @param processed_search_string
     * @param query_tokens
     * @param ignore_case
     * @param sub_query
     * @return SubQueryMatchabilityResult::SupercedesAllSubQueries
     * @return SubQueryMatchabilityResult::WontMatch
     * @return SubQueryMatchabilityResult::MayMatch
     */
    template <
            LogTypeDictionaryReaderReq LogTypeDictionaryReaderType,
            VariableDictionaryReaderReq VariableDictionaryReaderType
    >
    static SubQueryMatchabilityResult generate_logtypes_and_vars_for_subquery(
            LogTypeDictionaryReaderType const& logtype_dict,
            VariableDictionaryReaderType const& var_dict,
            std::string& processed_search_string,
            std::vector<QueryToken>& query_tokens,
            bool ignore_case,
            SubQuery& sub_query
    );

    /**
     * Normalizes a set of interpretations by collapsing consecutive greedy wildcards ('*') within
     * each token.
     *
     * Consecutive wildcards that span across the boundary of tokens are preserved.
     *
     * @param interpretations The original set of `QueryInterpretation`s to normalize.
     * @return The normalized set of `QueryInterpretation`s.
     */
    static auto normalize_interpretations(
            std::set<log_surgeon::wildcard_query_parser::QueryInterpretation> const& interpretations
    ) -> std::set<log_surgeon::wildcard_query_parser::QueryInterpretation>;

    /**
     * Compare all log-surgeon interpretations against the dictionaries to determine the sub queries
     * to search for within the archive.
     *
     * A. For each interpretation we must consider encodable wildcard variables (e.g. <int>(*1)).
     *    Each such variable introduces a binary choice:
     *    - 0: treat as a dictionary variable (\d)
     *    - 1: treat as an encoded variable (\i for integers, \f for floats)
     *
     *    If there are k encodable wildcard variables, then 2^k logtype strings are possible. As a
     *    result we limit k <= 16. We represent these alternatives using a bitmask.
     *
     *    Example:
     *      Search query: "a *1 *2 b",
     *      Interpretation (one of many): "a <int>(*1) <float>(*2) b"
     *      Possible logtypes (for the above interpretation):
     *        mask 00 -> "a \d \d b"
     *        mask 01 -> "a \d \f b"
     *        mask 10 -> "a \i \d b"
     *        mask 11 -> "a \i \f b"
     *
     * B. Each candidate combination becomes a useful subquery only if:
     *   1. The logtype exists in the logtype dictionary, and
     *   2. Each variable is either:
     *     a) resolvable in the variable dictionary (for dictionary vars), or
     *     b) encoded (always assumed valid).
     *
     *   Note: Encoded variables are always assumed to exist in the segment. This is a performance
     *   trade-off: checking the archive would be slower than decompressing.
     *
     * @tparam LogTypeDictionaryReaderType Logtype dictionary reader type.
     * @tparam VariableDictionaryReaderType Variable dictionary reader type.
     * @param interpretations Log-surgeon's interpretations of the search query.
     * @param logtype_dict The logtype dictionary.
     * @param var_dict The variable dictionary.
     * @param ignore_case If true, perform a case-insensitive search.
     * @param sub_queries Returns the subqueries to compare against CLP's archives.
     * @throw std::runtime_error If there are too many candidate combinations.
     */
    template <
            LogTypeDictionaryReaderReq LogTypeDictionaryReaderType,
            VariableDictionaryReaderReq VariableDictionaryReaderType
    >
    static auto generate_schema_sub_queries(
            std::set<log_surgeon::wildcard_query_parser::QueryInterpretation> const&
                    interpretations,
            LogTypeDictionaryReaderType const& logtype_dict,
            VariableDictionaryReaderType const& var_dict,
            bool ignore_case,
            std::vector<SubQuery>& sub_queries
    ) -> void;

    /**
     * Scans the interpretation and returns the indices of all encodable wildcard variables.
     *
     * An encodable variable is a variable token that:
     *   - Contains a wildcard (e.g. *1).
     *   - Is of an encodable type (integer or float).
     *
     * @param interpretation The `QueryInterpretation` to scan.
     * @return A vector of positions of encodable wildcard variables.
     */
    static auto get_wildcard_encodable_positions(
            log_surgeon::wildcard_query_parser::QueryInterpretation const& interpretation
    ) -> std::vector<size_t>;

    /**
     * Generates a logtype string from an interpretation, applying a mask to determine which
     * encodable wildcard positions are treated as encoded vs dictionary variables.
     *   - 0: Treat as dictionary variable.
     *   - 1: Treat as an encoded variable.
     *
     * @param interpretation The interpretation to convert to a logtype string.
     * @param wildcard_encodable_positions A vector of positions of encodable wildcard variables.
     * @param mask_encoded_flags A vector indicating if a variables is mask encoded.
     * @return The logtype string corresponding to this combination of encoded variables.
     */
    static auto generate_logtype_string(
            log_surgeon::wildcard_query_parser::QueryInterpretation const& interpretation,
            std::vector<size_t> const& wildcard_encodable_positions,
            std::vector<bool> const& mask_encoded_flags
    ) -> std::string;

    /**
     * Process a single variable token for schema subquery generation.
     *
     * Determines if the variable can be treated as:
     * - an encoded variable,
     * - a dictionary variable,
     * - or requires wildcard dictionary search.
     *
     * Updates `sub_query` with the appropriate variable encodings.
     *
     * @tparam VariableDictionaryReaderType Variable dictionary reader type.
     * @param variable_token The variable token to process.
     * @param var_dict The variable dictionary.
     * @param ignore_case If true, perform a case-insensitive search.
     * @param is_mask_encoded If the token is an encodable wildcard and is to be encoded.
     * @param sub_query Returns the updated sub query object.
     * @return True if the variable is encoded or is in the variable dictionary, false otherwise.
     */
    template <VariableDictionaryReaderReq VariableDictionaryReaderType>
    static auto process_schema_var_token(
            log_surgeon::wildcard_query_parser::VariableQueryToken const& variable_token,
            VariableDictionaryReaderType const& var_dict,
            bool ignore_case,
            bool is_mask_encoded,
            SubQuery& sub_query
    ) -> bool;
};

template <
        LogTypeDictionaryReaderReq LogTypeDictionaryReaderType,
        VariableDictionaryReaderReq VariableDictionaryReaderType
>
std::optional<Query> GrepCore::process_raw_query(
        LogTypeDictionaryReaderType const& logtype_dict,
        VariableDictionaryReaderType const& var_dict,
        std::string const& search_string,
        epochtime_t search_begin_ts,
        epochtime_t search_end_ts,
        bool ignore_case,
        log_surgeon::lexers::ByteLexer& lexer,
        bool use_heuristic
) {
    std::vector<SubQuery> sub_queries;
    if (use_heuristic) {
        // Split search_string into tokens with wildcards
        std::vector<QueryToken> query_tokens;
        size_t begin_pos = 0;
        size_t end_pos = 0;
        bool is_var;
        std::string search_string_for_sub_queries{search_string};

        // Replace unescaped '?' wildcards with '*' wildcards since we currently have no support for
        // generating sub-queries with '?' wildcards. The final wildcard match on the decompressed
        // message uses the original wildcards, so correctness will be maintained.
        string_utils::replace_unescaped_char(
                string_utils::cWildcardEscapeChar,
                string_utils::cSingleCharWildcard,
                string_utils::cZeroOrMoreCharsWildcard,
                search_string_for_sub_queries
        );

        // Clean-up in case any instances of "?*" or "*?" were changed into "**"
        search_string_for_sub_queries
                = string_utils::clean_up_wildcard_search_string(search_string_for_sub_queries);
        while (get_bounds_of_next_potential_var(
                search_string_for_sub_queries,
                begin_pos,
                end_pos,
                is_var
        ))
        {
            query_tokens.emplace_back(search_string_for_sub_queries, begin_pos, end_pos, is_var);
        }
        // Get pointers to all ambiguous tokens. Exclude tokens with wildcards in the middle since
        // we fall-back to decompression + wildcard matching for those.
        std::vector<QueryToken*> ambiguous_tokens;
        for (auto& query_token : query_tokens) {
            if (false == query_token.has_greedy_wildcard_in_middle()
                && query_token.is_ambiguous_token())
            {
                ambiguous_tokens.push_back(&query_token);
            }
        }

        // Generate a sub-query for each combination of ambiguous tokens
        // E.g., if there are two ambiguous tokens each of which could be a logtype or variable, we
        // need to create:
        // - (token1 as logtype) (token2 as logtype)
        // - (token1 as logtype) (token2 as var)
        // - (token1 as var) (token2 as logtype)
        // - (token1 as var) (token2 as var)
        std::string logtype;
        bool type_of_one_token_changed = true;
        while (type_of_one_token_changed) {
            SubQuery sub_query;

            // Compute logtypes and variables for query
            auto matchability = generate_logtypes_and_vars_for_subquery(
                    logtype_dict,
                    var_dict,
                    search_string_for_sub_queries,
                    query_tokens,
                    ignore_case,
                    sub_query
            );
            switch (matchability) {
                case SubQueryMatchabilityResult::SupercedesAllSubQueries:
                    // Since other sub-queries will be superceded by this one, we can stop
                    // processing now
                    return Query{search_begin_ts, search_end_ts, ignore_case, search_string, {}};
                case SubQueryMatchabilityResult::MayMatch:
                    sub_queries.push_back(std::move(sub_query));
                    break;
                case SubQueryMatchabilityResult::WontMatch:
                default:
                    // Do nothing
                    break;
            }

            // Update combination of ambiguous tokens
            type_of_one_token_changed = false;
            for (auto* ambiguous_token : ambiguous_tokens) {
                if (ambiguous_token->change_to_next_possible_type()) {
                    type_of_one_token_changed = true;
                    break;
                }
            }
        }
    } else {
        // TODO: Optimize such that interpretations are only generated once per schema.
        log_surgeon::wildcard_query_parser::Query const query{search_string};
        auto const interpretations{query.get_all_multi_token_interpretations(lexer)};
        auto const normalized_interpretations{normalize_interpretations(interpretations)};
        generate_schema_sub_queries(
                normalized_interpretations,
                logtype_dict,
                var_dict,
                ignore_case,
                sub_queries
        );
    }

    if (sub_queries.empty()) {
        return std::nullopt;
    }

    return Query{
            search_begin_ts,
            search_end_ts,
            ignore_case,
            search_string,
            std::move(sub_queries)
    };
}

template <VariableDictionaryReaderReq VariableDictionaryReaderType>
bool GrepCore::process_var_token(
        QueryToken const& query_token,
        VariableDictionaryReaderType const& var_dict,
        bool ignore_case,
        SubQuery& sub_query,
        std::string& logtype
) {
    // Even though we may have a precise variable, we still fallback to decompressing to ensure that
    // it is in the right place in the message
    sub_query.mark_wildcard_match_required();

    // Create QueryVar corresponding to token
    if (!query_token.contains_wildcards()) {
        if (EncodedVariableInterpreter::encode_and_search_dictionary(
                    query_token.get_value(),
                    var_dict,
                    ignore_case,
                    logtype,
                    sub_query
            )
            == false)
        {
            // Variable doesn't exist in dictionary
            return false;
        }
    } else {
        if (query_token.has_prefix_greedy_wildcard()) {
            logtype += '*';
        }

        if (query_token.is_float_var()) {
            EncodedVariableInterpreter::add_float_var(logtype);
        } else if (query_token.is_int_var()) {
            EncodedVariableInterpreter::add_int_var(logtype);
        } else {
            EncodedVariableInterpreter::add_dict_var(logtype);

            if (query_token.cannot_convert_to_non_dict_var()) {
                // Must be a dictionary variable, so search variable dictionary
                if (!EncodedVariableInterpreter::wildcard_search_dictionary_and_get_encoded_matches(
                            query_token.get_value(),
                            var_dict,
                            ignore_case,
                            sub_query
                    ))
                {
                    // Variable doesn't exist in dictionary
                    return false;
                }
            }
        }

        if (query_token.has_suffix_greedy_wildcard()) {
            logtype += '*';
        }
    }

    return true;
}

template <
        LogTypeDictionaryReaderReq LogTypeDictionaryReaderType,
        VariableDictionaryReaderReq VariableDictionaryReaderType
>
GrepCore::SubQueryMatchabilityResult GrepCore::generate_logtypes_and_vars_for_subquery(
        LogTypeDictionaryReaderType const& logtype_dict,
        VariableDictionaryReaderType const& var_dict,
        std::string& processed_search_string,
        std::vector<QueryToken>& query_tokens,
        bool ignore_case,
        SubQuery& sub_query
) {
    size_t last_token_end_pos = 0;
    std::string logtype;
    auto escape_handler =
            [](std::string_view constant, size_t char_to_escape_pos, std::string& logtype) -> void {
        auto const escape_char{enum_to_underlying_type(ir::VariablePlaceholder::Escape)};
        auto const next_char_pos{char_to_escape_pos + 1};
        // NOTE: We don't want to add additional escapes for wildcards that have been escaped. E.g.,
        // the query "\\*" should remain unchanged.
        if (next_char_pos < constant.length()
            && false == string_utils::is_wildcard(constant[next_char_pos]))
        {
            logtype += escape_char;
        } else if (ir::is_variable_placeholder(constant[char_to_escape_pos])) {
            logtype += escape_char;
            logtype += escape_char;
        }
    };
    for (auto const& query_token : query_tokens) {
        // Append from end of last token to beginning of this token, to logtype
        ir::append_constant_to_logtype(
                static_cast<std::string_view>(processed_search_string)
                        .substr(last_token_end_pos,
                                query_token.get_begin_pos() - last_token_end_pos),
                escape_handler,
                logtype
        );
        last_token_end_pos = query_token.get_end_pos();

        if (query_token.is_wildcard()) {
            logtype += '*';
        } else if (query_token.has_greedy_wildcard_in_middle()) {
            // Fallback to decompression + wildcard matching for now to avoid handling queries where
            // the pieces of the token on either side of each wildcard need to be processed as
            // ambiguous tokens
            sub_query.mark_wildcard_match_required();
            if (!query_token.is_var()) {
                logtype += '*';
            } else {
                logtype += '*';
                EncodedVariableInterpreter::add_dict_var(logtype);
                logtype += '*';
            }
        } else {
            if (!query_token.is_var()) {
                ir::append_constant_to_logtype(query_token.get_value(), escape_handler, logtype);
            } else if (!process_var_token(query_token, var_dict, ignore_case, sub_query, logtype)) {
                return SubQueryMatchabilityResult::WontMatch;
            }
        }
    }

    if (last_token_end_pos < processed_search_string.length()) {
        // Append from end of last token to end
        ir::append_constant_to_logtype(
                static_cast<std::string_view>(processed_search_string)
                        .substr(last_token_end_pos, std::string::npos),
                escape_handler,
                logtype
        );
        last_token_end_pos = processed_search_string.length();
    }

    if ("*" == logtype) {
        // Logtype will match all messages
        return SubQueryMatchabilityResult::SupercedesAllSubQueries;
    }

    // Find matching logtypes
    std::unordered_set<typename LogTypeDictionaryReaderType::Entry const*> possible_logtype_entries;
    logtype_dict
            .get_entries_matching_wildcard_string(logtype, ignore_case, possible_logtype_entries);
    if (possible_logtype_entries.empty()) {
        return SubQueryMatchabilityResult::WontMatch;
    }

    std::unordered_set<logtype_dictionary_id_t> possible_logtype_ids;
    for (auto const* entry : possible_logtype_entries) {
        possible_logtype_ids.emplace(entry->get_id());
    }
    sub_query.set_possible_logtypes(possible_logtype_ids);

    return SubQueryMatchabilityResult::MayMatch;
}

template <
        LogTypeDictionaryReaderReq LogTypeDictionaryReaderType,
        VariableDictionaryReaderReq VariableDictionaryReaderType
>
void GrepCore::generate_schema_sub_queries(
        std::set<log_surgeon::wildcard_query_parser::QueryInterpretation> const& interpretations,
        LogTypeDictionaryReaderType const& logtype_dict,
        VariableDictionaryReaderType const& var_dict,
        bool const ignore_case,
        std::vector<SubQuery>& sub_queries
) {
    constexpr size_t cMaxEncodableWildcardVariables{16};
    for (auto const& interpretation : interpretations) {
        auto const logtype{interpretation.get_logtype()};
        auto wildcard_encodable_positions{get_wildcard_encodable_positions(interpretation)};
        if (wildcard_encodable_positions.size() > cMaxEncodableWildcardVariables) {
            throw std::runtime_error("Too many encodable variables.");
        }
        uint64_t const num_combos{1ULL << wildcard_encodable_positions.size()};
        for (uint64_t mask{0}; mask < num_combos; ++mask) {
            std::vector<bool> mask_encoded_flags(logtype.size(), false);
            for (size_t i{0}; i < wildcard_encodable_positions.size(); ++i) {
                mask_encoded_flags[wildcard_encodable_positions[i]] = (mask >> i) & 1ULL;
            }

            auto logtype_string{generate_logtype_string(
                    interpretation,
                    wildcard_encodable_positions,
                    mask_encoded_flags
            )};

            std::unordered_set<typename LogTypeDictionaryReaderType::Entry const*> logtype_entries;
            logtype_dict.get_entries_matching_wildcard_string(
                    logtype_string,
                    ignore_case,
                    logtype_entries
            );
            if (logtype_entries.empty()) {
                continue;
            }

            SubQuery sub_query;
            bool has_vars{true};
            for (size_t i{0}; i < logtype.size(); ++i) {
                auto const& token{logtype[i]};
                if (std::holds_alternative<log_surgeon::wildcard_query_parser::VariableQueryToken>(
                            token
                    ))
                {
                    bool is_mask_encoded{false};
                    if (wildcard_encodable_positions.end()
                        != std::ranges::find(
                                wildcard_encodable_positions.begin(),
                                wildcard_encodable_positions.end(),
                                i
                        ))
                    {
                        is_mask_encoded = mask_encoded_flags[i];
                    }

                    has_vars = process_schema_var_token(
                            std::get<log_surgeon::wildcard_query_parser::VariableQueryToken>(token),
                            var_dict,
                            ignore_case,
                            is_mask_encoded,
                            sub_query
                    );
                }
                if (false == has_vars) {
                    break;
                }
            }
            if (false == has_vars) {
                continue;
            }

            std::unordered_set<logtype_dictionary_id_t> possible_logtype_ids;
            possible_logtype_ids.reserve(logtype_entries.size());
            for (auto const* entry : logtype_entries) {
                possible_logtype_ids.emplace(entry->get_id());
            }
            sub_query.set_possible_logtypes(possible_logtype_ids);
            sub_queries.push_back(std::move(sub_query));
        }
    }
}

template <VariableDictionaryReaderReq VariableDictionaryReaderType>
auto GrepCore::process_schema_var_token(
        log_surgeon::wildcard_query_parser::VariableQueryToken const& variable_token,
        VariableDictionaryReaderType const& var_dict,
        bool const ignore_case,
        bool const is_mask_encoded,
        SubQuery& sub_query
) -> bool {
    auto const& raw_string{variable_token.get_query_substring()};
    auto const var_has_wildcard{variable_token.get_contains_wildcard()};
    auto const var_type{static_cast<log_surgeon::SymbolId>(variable_token.get_variable_type())};
    bool const is_int{log_surgeon::SymbolId::TokenInt == var_type};
    bool const is_float{log_surgeon::SymbolId::TokenFloat == var_type};

    if (is_mask_encoded) {
        sub_query.mark_wildcard_match_required();
        return true;
    }

    if (var_has_wildcard) {
        return EncodedVariableInterpreter::wildcard_search_dictionary_and_get_encoded_matches(
                raw_string,
                var_dict,
                ignore_case,
                sub_query
        );
    }

    encoded_variable_t encoded_var{};
    if ((is_int
         && EncodedVariableInterpreter::convert_string_to_representable_integer_var(
                 raw_string,
                 encoded_var
         ))
        || (is_float
            && EncodedVariableInterpreter::convert_string_to_representable_float_var(
                    raw_string,
                    encoded_var
            )))
    {
        sub_query.add_non_dict_var(encoded_var);
        return true;
    }

    auto entries = var_dict.get_entry_matching_value(raw_string, ignore_case);
    if (entries.empty()) {
        return false;
    }
    if (1 == entries.size()) {
        auto const entry_id{entries[0]->get_id()};
        sub_query.add_dict_var(EncodedVariableInterpreter::encode_var_dict_id(entry_id), entry_id);
        return true;
    }
    std::unordered_set<encoded_variable_t> encoded_vars;
    std::unordered_set<variable_dictionary_id_t> var_dict_ids;
    encoded_vars.reserve(entries.size());
    var_dict_ids.reserve(entries.size());
    for (auto const* entry : entries) {
        encoded_vars.emplace(EncodedVariableInterpreter::encode_var_dict_id(entry->get_id()));
        var_dict_ids.emplace(entry->get_id());
    }
    sub_query.add_imprecise_dict_var(encoded_vars, var_dict_ids);
    return true;
}
}  // namespace clp

#endif  // CLP_GREPCORE_HPP
