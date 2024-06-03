// Code from CLP

#include "Grep.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include "../../Utils.hpp"
#include "../../VariableEncoder.hpp"
#include "EncodedVariableInterpreter.hpp"

using std::string;
using std::vector;

namespace clp_s::search::clp_search {
// Local types
enum class SubQueryMatchabilityResult {
    MayMatch,  // The subquery might match a message
    WontMatch,  // The subquery has no chance of matching a message
    SupercedesAllSubQueries  // The subquery will cause all messages to be matched
};

// Class representing a token in a query. It is used to interpret a token in user's search
// string.
class QueryToken {
public:
    // Constructors
    QueryToken(string const& query_string, size_t begin_pos, size_t end_pos, bool is_var);

    // Methods
    bool cannot_convert_to_non_dict_var() const;
    bool contains_wildcards() const;
    bool has_greedy_wildcard_in_middle() const;
    bool has_prefix_greedy_wildcard() const;
    bool has_suffix_greedy_wildcard() const;
    bool is_ambiguous_token() const;
    bool is_double_var() const;
    bool is_var() const;
    bool is_wildcard() const;

    size_t get_begin_pos() const;
    size_t get_end_pos() const;
    string const& get_value() const;

    bool change_to_next_possible_type();

private:
    // Types
    // Type for the purpose of generating different subqueries. E.g., if a token is of type
    // DictOrIntVar, it would generate a different subquery than if it was of type Logtype.
    enum class Type {
        Wildcard,
        // Ambiguous indicates the token can be more than one of the types listed below
        Ambiguous,
        Logtype,
        DictOrIntVar,
        DoubleVar
    };

    // Variables
    bool m_cannot_convert_to_non_dict_var;
    bool m_contains_wildcards;
    bool m_has_greedy_wildcard_in_middle;
    bool m_has_prefix_greedy_wildcard;
    bool m_has_suffix_greedy_wildcard;

    size_t m_begin_pos;
    size_t m_end_pos;
    string m_value;

    // Type if variable has unambiguous type
    Type m_type;
    // Types if variable type is ambiguous
    vector<Type> m_possible_types;
    // Index of the current possible type selected for generating a subquery
    size_t m_current_possible_type_ix;
};

QueryToken::QueryToken(
        string const& query_string,
        size_t const begin_pos,
        size_t const end_pos,
        bool const is_var
)
        : m_current_possible_type_ix(0) {
    m_begin_pos = begin_pos;
    m_end_pos = end_pos;
    m_value.assign(query_string, m_begin_pos, m_end_pos - m_begin_pos);

    // Set wildcard booleans and determine type
    if ("*" == m_value) {
        m_has_prefix_greedy_wildcard = true;
        m_has_suffix_greedy_wildcard = false;
        m_has_greedy_wildcard_in_middle = false;
        m_contains_wildcards = true;
        m_type = Type::Wildcard;
    } else {
        m_has_prefix_greedy_wildcard = ('*' == m_value[0]);
        m_has_suffix_greedy_wildcard = ('*' == m_value[m_value.length() - 1]);

        m_has_greedy_wildcard_in_middle = false;
        for (size_t i = 1; i < m_value.length() - 1; ++i) {
            if ('*' == m_value[i]) {
                m_has_greedy_wildcard_in_middle = true;
                break;
            }
        }

        m_contains_wildcards
                = (m_has_prefix_greedy_wildcard || m_has_suffix_greedy_wildcard
                   || m_has_greedy_wildcard_in_middle);

        if (false == is_var) {
            if (false == m_contains_wildcards) {
                m_type = Type::Logtype;
            } else {
                m_type = Type::Ambiguous;
                m_possible_types.push_back(Type::Logtype);
                m_possible_types.push_back(Type::DictOrIntVar);
                m_possible_types.push_back(Type::DoubleVar);
            }
        } else {
            string value_without_wildcards = m_value;
            if (m_has_prefix_greedy_wildcard) {
                value_without_wildcards = value_without_wildcards.substr(1);
            }
            if (m_has_suffix_greedy_wildcard) {
                value_without_wildcards.resize(value_without_wildcards.length() - 1);
            }

            encoded_variable_t encoded_var;
            bool converts_to_non_dict_var = false;
            if (VariableEncoder::convert_string_to_representable_integer_var(
                        value_without_wildcards,
                        encoded_var
                )
                || VariableEncoder::convert_string_to_representable_double_var(
                        value_without_wildcards,
                        encoded_var
                ))
            {
                converts_to_non_dict_var = true;
            }

            if (false == converts_to_non_dict_var) {
                // Dictionary variable
                m_type = Type::DictOrIntVar;
                m_cannot_convert_to_non_dict_var = true;
            } else {
                m_type = Type::Ambiguous;
                m_possible_types.push_back(Type::DictOrIntVar);
                m_possible_types.push_back(Type::DoubleVar);
                m_cannot_convert_to_non_dict_var = false;
            }
        }
    }
}

bool QueryToken::cannot_convert_to_non_dict_var() const {
    return m_cannot_convert_to_non_dict_var;
}

bool QueryToken::contains_wildcards() const {
    return m_contains_wildcards;
}

bool QueryToken::has_greedy_wildcard_in_middle() const {
    return m_has_greedy_wildcard_in_middle;
}

bool QueryToken::has_prefix_greedy_wildcard() const {
    return m_has_prefix_greedy_wildcard;
}

bool QueryToken::has_suffix_greedy_wildcard() const {
    return m_has_suffix_greedy_wildcard;
}

bool QueryToken::is_ambiguous_token() const {
    return Type::Ambiguous == m_type;
}

bool QueryToken::is_double_var() const {
    Type type;
    if (Type::Ambiguous == m_type) {
        type = m_possible_types[m_current_possible_type_ix];
    } else {
        type = m_type;
    }
    return Type::DoubleVar == type;
}

bool QueryToken::is_var() const {
    Type type;
    if (Type::Ambiguous == m_type) {
        type = m_possible_types[m_current_possible_type_ix];
    } else {
        type = m_type;
    }
    return (Type::DictOrIntVar == type || Type::DoubleVar == type);
}

bool QueryToken::is_wildcard() const {
    return Type::Wildcard == m_type;
}

size_t QueryToken::get_begin_pos() const {
    return m_begin_pos;
}

size_t QueryToken::get_end_pos() const {
    return m_end_pos;
}

string const& QueryToken::get_value() const {
    return m_value;
}

bool QueryToken::change_to_next_possible_type() {
    if (m_current_possible_type_ix < m_possible_types.size() - 1) {
        ++m_current_possible_type_ix;
        return true;
    } else {
        m_current_possible_type_ix = 0;
        return false;
    }
}

// Local prototypes
/**
 * Process a QueryToken that is definitely a variable
 * @param query_token
 * @param archive
 * @param ignore_case
 * @param sub_query
 * @param logtype
 * @return true if this token might match a message, false otherwise
 */
static bool process_var_token(
        QueryToken const& query_token,
        std::shared_ptr<VariableDictionaryReader> var_dict, /*const Archive& archive,*/
        bool ignore_case,
        SubQuery& sub_query,
        string& logtype
);
/**
 * Finds a message matching the given query
 * @param query
 * @param archive
 * @param matching_sub_query
 * @param compressed_file
 * @param compressed_msg
 * @return true on success, false otherwise
 */
// static bool find_matching_message (const Query& query, Archive& archive, const SubQuery*&
// matching_sub_query, File& compressed_file, Message& compressed_msg);
/**
 * Generates logtypes and variables for subquery
 * @param archive
 * @param processed_search_string
 * @param query_tokens
 * @param ignore_case
 * @param sub_query
 * @return SubQueryMatchabilityResult::SupercedesAllSubQueries
 * @return SubQueryMatchabilityResult::WontMatch
 * @return SubQueryMatchabilityResult::MayMatch
 */
static SubQueryMatchabilityResult generate_logtypes_and_vars_for_subquery(
        std::shared_ptr<LogTypeDictionaryReader> log_dict,
        std::shared_ptr<VariableDictionaryReader> var_dict, /*const Archive& archive,*/
        string& processed_search_string,
        vector<QueryToken>& query_tokens,
        bool ignore_case,
        SubQuery& sub_query
);

static bool process_var_token(
        QueryToken const& query_token,
        std::shared_ptr<VariableDictionaryReader> var_dict, /*const Archive& archive,*/
        bool ignore_case,
        SubQuery& sub_query,
        string& logtype
) {
    // Even though we may have a precise variable, we still fallback to decompressing to ensure
    // that it is in the right place in the message
    sub_query.mark_wildcard_match_required();

    // Create QueryVar corresponding to token
    if (false == query_token.contains_wildcards()) {
        if (EncodedVariableInterpreter::encode_and_search_dictionary(
                    query_token.get_value(),
                    *var_dict,
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

        if (query_token.is_double_var()) {
            LogTypeDictionaryEntry::add_double_var(logtype);
        } else {
            LogTypeDictionaryEntry::add_non_double_var(logtype);

            if (query_token.cannot_convert_to_non_dict_var()) {
                // Must be a dictionary variable, so search variable dictionary
                if (!EncodedVariableInterpreter::wildcard_search_dictionary_and_get_encoded_matches(
                            query_token.get_value(),
                            *var_dict,
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

SubQueryMatchabilityResult generate_logtypes_and_vars_for_subquery(
        std::shared_ptr<LogTypeDictionaryReader> log_dict,
        std::shared_ptr<VariableDictionaryReader> var_dict, /*const Archive& archive,*/
        string& processed_search_string,
        vector<QueryToken>& query_tokens,
        bool ignore_case,
        SubQuery& sub_query
) {
    size_t last_token_end_pos = 0;
    string logtype;
    for (auto const& query_token : query_tokens) {
        // Append from end of last token to beginning of this token, to logtype
        logtype.append(
                processed_search_string,
                last_token_end_pos,
                query_token.get_begin_pos() - last_token_end_pos
        );
        last_token_end_pos = query_token.get_end_pos();

        if (query_token.is_wildcard()) {
            logtype += '*';
        } else if (query_token.has_greedy_wildcard_in_middle()) {
            // Fallback to decompression + wildcard matching for now to avoid handling queries
            // where the pieces of the token on either side of each wildcard need to be
            // processed as ambiguous tokens
            sub_query.mark_wildcard_match_required();
            if (false == query_token.is_var()) {
                logtype += '*';
            } else {
                logtype += '*';
                LogTypeDictionaryEntry::add_non_double_var(logtype);
                logtype += '*';
            }
        } else {
            if (false == query_token.is_var()) {
                logtype += query_token.get_value();
            } else if (false
                       == process_var_token(query_token, var_dict, ignore_case, sub_query, logtype))
            {
                return SubQueryMatchabilityResult::WontMatch;
            }
        }
    }

    if (last_token_end_pos < processed_search_string.length()) {
        // Append from end of last token to end
        logtype.append(processed_search_string, last_token_end_pos, string::npos);
        last_token_end_pos = processed_search_string.length();
    }

    if ("*" == logtype) {
        // Logtype will match all messages
        return SubQueryMatchabilityResult::SupercedesAllSubQueries;
    }

    // Find matching logtypes
    std::unordered_set<LogTypeDictionaryEntry const*> possible_logtype_entries;
    log_dict->get_entries_matching_wildcard_string(logtype, ignore_case, possible_logtype_entries);
    if (possible_logtype_entries.empty()) {
        return SubQueryMatchabilityResult::WontMatch;
    }
    sub_query.set_possible_logtypes(possible_logtype_entries);

    // Calculate the IDs of the segments that may contain results for the sub-query now that
    // we've calculated the matching logtypes and variables
    // TODO: double check that this can be safely ignored for CLJ
    // sub_query.calculate_ids_of_matching_segments();

    return SubQueryMatchabilityResult::MayMatch;
}

std::optional<Query> Grep::process_raw_query(
        std::shared_ptr<LogTypeDictionaryReader> log_dict,
        std::shared_ptr<VariableDictionaryReader> var_dict,
        string const& search_string,
        bool ignore_case,
        bool add_wildcards
) {
    // Add prefix and suffix '*' to make the search a sub-string match
    string processed_search_string;
    if (add_wildcards) {
        processed_search_string = "*";
        processed_search_string += search_string;
        processed_search_string += '*';
    } else {
        processed_search_string = search_string;
    }

    // Clean-up search string
    processed_search_string = StringUtils::clean_up_wildcard_search_string(processed_search_string);

    // Replace non-greedy wildcards with greedy wildcards since we currently have no support for
    // searching compressed files with non-greedy wildcards
    std::replace(processed_search_string.begin(), processed_search_string.end(), '?', '*');
    // Clean-up in case any instances of "?*" or "*?" were changed into "**"
    processed_search_string = StringUtils::clean_up_wildcard_search_string(processed_search_string);

    // Split search_string into tokens with wildcards
    vector<QueryToken> query_tokens;
    size_t begin_pos = 0;
    size_t end_pos = 0;
    bool is_var;
    while (get_bounds_of_next_potential_var(processed_search_string, begin_pos, end_pos, is_var)) {
        query_tokens.emplace_back(processed_search_string, begin_pos, end_pos, is_var);
    }

    // Get pointers to all ambiguous tokens. Exclude tokens with wildcards in the middle since
    // we fall-back to decompression + wildcard matching for those.
    vector<QueryToken*> ambiguous_tokens;
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
    vector<SubQuery> sub_queries;
    string logtype;
    bool type_of_one_token_changed = true;
    while (type_of_one_token_changed) {
        SubQuery sub_query;

        // Compute logtypes and variables for query
        auto matchability = generate_logtypes_and_vars_for_subquery(
                log_dict,
                var_dict,
                processed_search_string,
                query_tokens,
                ignore_case,
                sub_query
        );
        switch (matchability) {
            case SubQueryMatchabilityResult::SupercedesAllSubQueries:
                // Since other sub-queries will be superceded by this one, we can stop
                // processing now
                return Query{ignore_case, processed_search_string, {}};
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

    if (sub_queries.empty()) {
        return std::nullopt;
    }

    return Query{ignore_case, processed_search_string, std::move(sub_queries)};
}

bool Grep::get_bounds_of_next_potential_var(
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

                if (StringUtils::is_delim(c)) {
                    // Found escaped non-delimiter, so reverse the index to retain the escape
                    // character
                    --begin_pos;
                    break;
                }
            } else if ('\\' == c) {
                // Escape character
                is_escaped = true;
            } else {
                if (StringUtils::is_wildcard(c)) {
                    contains_wildcard = true;
                    break;
                }
                if (false == StringUtils::is_delim(c)) {
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

                if (StringUtils::is_delim(c)) {
                    // Found escaped delimiter, so reverse the index to retain the escape
                    // character
                    --end_pos;
                    break;
                }
            } else if ('\\' == c) {
                // Escape character
                is_escaped = true;
            } else {
                if (StringUtils::is_wildcard(c)) {
                    contains_wildcard = true;
                } else if (StringUtils::is_delim(c)) {
                    // Found delimiter that's not also a wildcard
                    break;
                }
            }

            if (StringUtils::is_decimal_digit(c)) {
                contains_decimal_digit = true;
            } else if (StringUtils::is_alphabet(c)) {
                contains_alphabet = true;
            }
        }

        // Treat token as a definite variable if:
        // - it contains a decimal digit, or
        // - it could be a multi-digit hex value, or
        // - it's directly preceded by an equals sign and contains an alphabet without a
        // wildcard between the equals sign and the first alphabet of the token
        if (contains_decimal_digit
            || StringUtils::could_be_multi_digit_hex_value(value, begin_pos, end_pos))
        {
            is_var = true;
        } else if (begin_pos > 0 && '=' == value[begin_pos - 1] && contains_alphabet) {
            // Find first alphabet or wildcard in token
            is_escaped = false;
            bool found_wildcard_before_alphabet = false;
            for (auto i = begin_pos; i < end_pos; ++i) {
                auto c = value[i];

                if (is_escaped) {
                    is_escaped = false;

                    if (StringUtils::is_alphabet(c)) {
                        break;
                    }
                } else if ('\\' == c) {
                    // Escape character
                    is_escaped = true;
                } else if (StringUtils::is_wildcard(c)) {
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
}  // namespace clp_s::search::clp_search
