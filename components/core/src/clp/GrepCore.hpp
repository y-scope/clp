#ifndef CLP_GREPCORE_HPP
#define CLP_GREPCORE_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <log_surgeon/Lexer.hpp>
#include <string_utils/constants.hpp>
#include <string_utils/string_utils.hpp>

#include <clp/Defs.h>
#include <clp/EncodedVariableInterpreter.hpp>
#include <clp/ir/parsing.hpp>
#include <clp/ir/types.hpp>
#include <clp/LogTypeDictionaryReaderReq.hpp>
#include <clp/Query.hpp>
#include <clp/QueryToken.hpp>
#include <clp/SchemaSearcher.hpp>
#include <clp/VariableDictionaryReaderReq.hpp>

namespace clp {
class GrepCore {
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
    if (false == use_heuristic) {
        sub_queries
                = SchemaSearcher::search(search_string, lexer, logtype_dict, var_dict, ignore_case);
    } else {
        // Split search_string into tokens with wildcards
        std::vector<QueryToken> query_tokens;
        size_t begin_pos{0};
        size_t end_pos{0};
        bool is_var{false};
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
        bool type_of_one_token_changed{true};
        while (type_of_one_token_changed) {
            SubQuery sub_query;
            auto matchability{generate_logtypes_and_vars_for_subquery(
                    logtype_dict,
                    var_dict,
                    search_string_for_sub_queries,
                    query_tokens,
                    ignore_case,
                    sub_query
            )};
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
}  // namespace clp

#endif  // CLP_GREPCORE_HPP
