#include "Grep.hpp"

// C++ libraries
#include <algorithm>

// Project headers
#include "EncodedVariableInterpreter.hpp"
#include "Utils.hpp"

using std::string;
using std::vector;
using streaming_archive::reader::Archive;
using streaming_archive::reader::File;
using streaming_archive::reader::Message;

// Local types
enum class SubQueryMatchabilityResult {
    MayMatch, // The subquery might match a message
    WontMatch, // The subquery has no chance of matching a message
    SupercedesAllSubQueries // The subquery will cause all messages to be matched
};

// Class representing a token in a query. It is used to interpret a token in user's search string.
class QueryToken {
public:
    // Constructors
    QueryToken (const string& query_string, size_t begin_pos, size_t end_pos, bool is_var);

    // Methods
    bool cannot_convert_to_non_dict_var () const;
    bool contains_wildcards () const;
    bool has_greedy_wildcard_in_middle () const;
    bool has_prefix_greedy_wildcard () const;
    bool has_suffix_greedy_wildcard () const;
    bool is_ambiguous_token () const;
    bool is_double_var () const;
    bool is_var () const;
    bool is_wildcard () const;

    size_t get_begin_pos () const;
    size_t get_end_pos () const;
    const string& get_value () const;

    bool change_to_next_possible_type ();

private:
    // Types
    // Type for the purpose of generating different subqueries. E.g., if a token is of type DictOrIntVar, it would generate a different subquery than
    // if it was of type Logtype.
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

QueryToken::QueryToken (const string& query_string, const size_t begin_pos, const size_t end_pos, const bool is_var) : m_current_possible_type_ix(0) {
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

        m_contains_wildcards = (m_has_prefix_greedy_wildcard || m_has_suffix_greedy_wildcard || m_has_greedy_wildcard_in_middle);

        if (!is_var) {
            if (!m_contains_wildcards) {
                m_type = Type::Logtype;
            } else {
                m_type = Type::Ambiguous;
                m_possible_types.push_back(Type::Logtype);
                m_possible_types.push_back(Type::DictOrIntVar);
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
            if (EncodedVariableInterpreter::convert_string_to_representable_integer_var(value_without_wildcards, encoded_var) ||
                EncodedVariableInterpreter::convert_string_to_representable_double_var(value_without_wildcards, encoded_var))
            {
                converts_to_non_dict_var = true;
            }

            if (!converts_to_non_dict_var) {
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

bool QueryToken::cannot_convert_to_non_dict_var () const {
    return m_cannot_convert_to_non_dict_var;
}

bool QueryToken::contains_wildcards () const {
    return m_contains_wildcards;
}

bool QueryToken::has_greedy_wildcard_in_middle () const {
    return m_has_greedy_wildcard_in_middle;
}

bool QueryToken::has_prefix_greedy_wildcard () const {
    return m_has_prefix_greedy_wildcard;
}

bool QueryToken::has_suffix_greedy_wildcard () const {
    return m_has_suffix_greedy_wildcard;
}

bool QueryToken::is_ambiguous_token () const {
    return Type::Ambiguous == m_type;
}

bool QueryToken::is_double_var () const {
    Type type;
    if (Type::Ambiguous == m_type) {
        type = m_possible_types[m_current_possible_type_ix];
    } else {
        type = m_type;
    }
    return Type::DoubleVar == type;
}

bool QueryToken::is_var () const {
    Type type;
    if (Type::Ambiguous == m_type) {
        type = m_possible_types[m_current_possible_type_ix];
    } else {
        type = m_type;
    }
    return (Type::DictOrIntVar == type || Type::DoubleVar == type);
}

bool QueryToken::is_wildcard () const {
    return Type::Wildcard == m_type;
}

size_t QueryToken::get_begin_pos () const {
    return m_begin_pos;
}

size_t QueryToken::get_end_pos () const {
    return m_end_pos;
}

const string& QueryToken::get_value () const {
    return m_value;
}

bool QueryToken::change_to_next_possible_type () {
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
static bool process_var_token (const QueryToken& query_token, const Archive& archive, bool ignore_case, SubQuery& sub_query, string& logtype);
/**
 * Finds a message matching the given query
 * @param query
 * @param archive
 * @param matching_sub_query
 * @param compressed_file
 * @param compressed_msg
 * @return true on success, false otherwise
 */
static bool find_matching_message (const Query& query, Archive& archive, const SubQuery*& matching_sub_query, File& compressed_file, Message& compressed_msg);
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
static SubQueryMatchabilityResult generate_logtypes_and_vars_for_subquery (const Archive& archive, string& processed_search_string,
                                                                           vector<QueryToken>& query_tokens, bool ignore_case, SubQuery& sub_query);

static bool process_var_token (const QueryToken& query_token, const Archive& archive, bool ignore_case, SubQuery& sub_query, string& logtype) {
    // Even though we may have a precise variable, we still fallback to decompressing to ensure that it is in the right place in the message
    sub_query.mark_wildcard_match_required();

    // Create QueryVar corresponding to token
    if (!query_token.contains_wildcards()) {
        if (EncodedVariableInterpreter::encode_and_search_dictionary(query_token.get_value(), archive.get_var_dictionary(), ignore_case, logtype,
                                                                     sub_query) == false)
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
                if (!EncodedVariableInterpreter::wildcard_search_dictionary_and_get_encoded_matches(query_token.get_value(), archive.get_var_dictionary(),
                                                                                                    ignore_case, sub_query))
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

static bool find_matching_message (const Query& query, Archive& archive, const SubQuery*& matching_sub_query, File& compressed_file, Message& compressed_msg) {
    if (query.contains_sub_queries()) {
        matching_sub_query = archive.find_message_matching_query(compressed_file, query, compressed_msg);
        if (nullptr == matching_sub_query) {
            return false;
        }
    } else if (query.get_search_begin_timestamp() > cEpochTimeMin || query.get_search_end_timestamp() < cEpochTimeMax) {
        bool found_msg = archive.find_message_in_time_range(compressed_file, query.get_search_begin_timestamp(), query.get_search_end_timestamp(),
                                                            compressed_msg);
        if (!found_msg) {
            return false;
        }
    } else {
        bool read_successful = archive.get_next_message(compressed_file, compressed_msg);
        if (!read_successful) {
            return false;
        }
    }

    return true;
}

SubQueryMatchabilityResult generate_logtypes_and_vars_for_subquery (const Archive& archive, string& processed_search_string, vector<QueryToken>& query_tokens,
                                                                    bool ignore_case, SubQuery& sub_query)
{
    size_t last_token_end_pos = 0;
    string logtype;
    for (const auto& query_token : query_tokens) {
        // Append from end of last token to beginning of this token, to logtype
        logtype.append(processed_search_string, last_token_end_pos, query_token.get_begin_pos() - last_token_end_pos);
        last_token_end_pos = query_token.get_end_pos();

        if (query_token.is_wildcard()) {
            logtype += '*';
        } else if (query_token.has_greedy_wildcard_in_middle()) {
            // Fallback to decompression + wildcard matching for now to avoid handling queries where the pieces of the token on either side of each wildcard
            // need to be processed as ambiguous tokens
            sub_query.mark_wildcard_match_required();
            if (!query_token.is_var()) {
                logtype += '*';
            } else {
                logtype += '*';
                LogTypeDictionaryEntry::add_non_double_var(logtype);
                logtype += '*';
            }
        } else {
            if (!query_token.is_var()) {
                logtype += query_token.get_value();
            } else if (!process_var_token(query_token, archive, ignore_case, sub_query, logtype)) {
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
    std::unordered_set<const LogTypeDictionaryEntry*> possible_logtype_entries;
    archive.get_logtype_dictionary().get_entries_matching_wildcard_string(logtype, ignore_case, possible_logtype_entries);
    if (possible_logtype_entries.empty()) {
        return SubQueryMatchabilityResult::WontMatch;
    }
    sub_query.set_possible_logtypes(possible_logtype_entries);

    // Calculate the IDs of the segments that may contain results for the sub-query now that we've calculated the matching logtypes and variables
    sub_query.calculate_ids_of_matching_segments();

    return SubQueryMatchabilityResult::MayMatch;
}

bool Grep::process_raw_query (const Archive& archive, const string& search_string, epochtime_t search_begin_ts, epochtime_t search_end_ts, bool ignore_case,
        Query& query)
{
    // Set properties which require no processing
    query.set_search_begin_timestamp(search_begin_ts);
    query.set_search_end_timestamp(search_end_ts);
    query.set_ignore_case(ignore_case);

    // Clean-up search string
    string processed_search_string = clean_up_wildcard_search_string(search_string);
    query.set_search_string(processed_search_string);

    // Replace non-greedy wildcards with greedy wildcards since we currently have no support for searching compressed files with non-greedy wildcards
    std::replace(processed_search_string.begin(), processed_search_string.end(), '?', '*');
    // Clean-up in case any instances of "?*" or "*?" were changed into "**"
    processed_search_string = clean_up_wildcard_search_string(processed_search_string);

    // Split search_string into tokens with wildcards
    vector<QueryToken> query_tokens;
    size_t begin_pos = 0;
    size_t end_pos = 0;
    bool is_var;
    while (get_bounds_of_next_potential_var(processed_search_string, begin_pos, end_pos, is_var)) {
        query_tokens.emplace_back(processed_search_string, begin_pos, end_pos, is_var);
    }

    // Get pointers to all ambiguous tokens. Exclude tokens with wildcards in the middle since we fall-back to decompression + wildcard matching for those.
    vector<QueryToken*> ambiguous_tokens;
    for (auto& query_token : query_tokens) {
        if (!query_token.has_greedy_wildcard_in_middle() && query_token.is_ambiguous_token()) {
            ambiguous_tokens.push_back(&query_token);
        }
    }

    // Generate a sub-query for each combination of ambiguous tokens
    // E.g., if there are two ambiguous tokens each of which could be a logtype or variable, we need to create:
    // - (token1 as logtype) (token2 as logtype)
    // - (token1 as logtype) (token2 as var)
    // - (token1 as var) (token2 as logtype)
    // - (token1 as var) (token2 as var)
    SubQuery sub_query;
    string logtype;
    bool type_of_one_token_changed = true;
    while (type_of_one_token_changed) {
        sub_query.clear();

        // Compute logtypes and variables for query
        auto matchability = generate_logtypes_and_vars_for_subquery(archive, processed_search_string, query_tokens, query.get_ignore_case(), sub_query);
        switch (matchability) {
            case SubQueryMatchabilityResult::SupercedesAllSubQueries:
                // Clear all sub-queries since they will be superceded by this sub-query
                query.clear_sub_queries();

                // Since other sub-queries will be superceded by this one, we can stop processing now
                return true;
            case SubQueryMatchabilityResult::MayMatch:
                query.add_sub_query(sub_query);
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

    return query.contains_sub_queries();
}

void Grep::calculate_sub_queries_relevant_to_file (const File& compressed_file, vector<Query>& queries) {
    if (compressed_file.is_in_segment()) {
        for (auto& query : queries) {
            query.make_sub_queries_relevant_to_segment(compressed_file.get_segment_id());
        }
    } else {
        for (auto& query : queries) {
            query.make_all_sub_queries_relevant();
        }
    }
}

size_t Grep::search_and_output (const Query& query, size_t limit, Archive& archive, File& compressed_file, OutputFunc output_func, void* output_func_arg) {
    size_t num_matches = 0;

    Message compressed_msg;
    string decompressed_msg;
    const string& orig_file_path = compressed_file.get_orig_path();
    while (num_matches < limit) {
        // Find matching message
        const SubQuery* matching_sub_query = nullptr;
        if (find_matching_message(query, archive, matching_sub_query, compressed_file, compressed_msg) == false) {
            break;
        }

        // Decompress match
        bool decompress_successful = archive.decompress_message(compressed_file, compressed_msg, decompressed_msg);
        if (!decompress_successful) {
            break;
        }

        // Perform wildcard match if required
        // Check if:
        // - Sub-query requires wildcard match, or
        // - no subqueries exist and the search string is not a match-all
        if ((query.contains_sub_queries() && matching_sub_query->wildcard_match_required()) ||
            (query.contains_sub_queries() == false && query.search_string_matches_all() == false))
        {
            bool matched = wildCardMatch(decompressed_msg, query.get_search_string(), query.get_ignore_case() == false);
            if (!matched) {
                continue;
            }
        }

        // Print match
        output_func(orig_file_path, compressed_msg, decompressed_msg, output_func_arg);
        ++num_matches;
    }

    return num_matches;
}

bool Grep::search_and_decompress (const Query& query, Archive& archive, File& compressed_file, Message& compressed_msg, string& decompressed_msg) {
    const string& orig_file_path = compressed_file.get_orig_path();

    bool matched = false;
    while (false == matched) {
        // Find matching message
        const SubQuery* matching_sub_query = nullptr;
        bool message_found = find_matching_message(query, archive, matching_sub_query, compressed_file, compressed_msg);
        if (false == message_found) {
            return false;
        }

        // Decompress match
        bool decompress_successful = archive.decompress_message(compressed_file, compressed_msg, decompressed_msg);
        if (false == decompress_successful) {
            return false;
        }

        // Perform wildcard match if required
        // Check if:
        // - Sub-query requires wildcard match, or
        // - no subqueries exist and the search string is not a match-all
        if ((query.contains_sub_queries() && matching_sub_query->wildcard_match_required()) ||
            (query.contains_sub_queries() == false && query.search_string_matches_all() == false))
        {
            matched = wildCardMatch(decompressed_msg, query.get_search_string(), query.get_ignore_case() == false);
        } else {
            matched = true;
        }
    }

    return true;
}

size_t Grep::search (const Query& query, size_t limit, Archive& archive, File& compressed_file) {
    size_t num_matches = 0;

    Message compressed_msg;
    string decompressed_msg;
    const string& orig_file_path = compressed_file.get_orig_path();
    while (num_matches < limit) {
        // Find matching message
        const SubQuery* matching_sub_query = nullptr;
        if (find_matching_message(query, archive, matching_sub_query, compressed_file, compressed_msg) == false) {
            break;
        }

        // Perform wildcard match if required
        // Check if:
        // - Sub-query requires wildcard match, or
        // - no subqueries exist and the search string is not a match-all
        if ((query.contains_sub_queries() && matching_sub_query->wildcard_match_required()) ||
            (query.contains_sub_queries() == false && query.search_string_matches_all() == false))
        {
            // Decompress match
            bool decompress_successful = archive.decompress_message(compressed_file, compressed_msg, decompressed_msg);
            if (!decompress_successful) {
                break;
            }

            bool matched = wildCardMatch(decompressed_msg, query.get_search_string(), query.get_ignore_case() == false);
            if (!matched) {
                continue;
            }
        }

        ++num_matches;
    }

    return num_matches;
}
