#include "Grep.hpp"

#include <algorithm>

#include <log_surgeon/Constants.hpp>
#include <string_utils/string_utils.hpp>

#include "EncodedVariableInterpreter.hpp"
#include "ir/parsing.hpp"
#include "ir/types.hpp"
#include "LogSurgeonReader.hpp"
#include "StringReader.hpp"
#include "Utils.hpp"

using clp::ir::is_delim;
using clp::streaming_archive::reader::Archive;
using clp::streaming_archive::reader::File;
using clp::streaming_archive::reader::Message;
using clp::string_utils::clean_up_wildcard_search_string;
using clp::string_utils::is_alphabet;
using clp::string_utils::is_wildcard;
using clp::string_utils::wildcard_match_unsafe;
using std::string;
using std::vector;

namespace clp {
namespace {
// Local types
enum class SubQueryMatchabilityResult {
    MayMatch,  // The subquery might match a message
    WontMatch,  // The subquery has no chance of matching a message
    SupercedesAllSubQueries  // The subquery will cause all messages to be matched
};

// Class representing a token in a query. It is used to interpret a token in user's search string.
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
    bool is_float_var() const;
    bool is_int_var() const;
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
        DictionaryVar,
        FloatVar,
        IntVar
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

        if (!is_var) {
            if (!m_contains_wildcards) {
                m_type = Type::Logtype;
            } else {
                m_type = Type::Ambiguous;
                m_possible_types.push_back(Type::Logtype);
                m_possible_types.push_back(Type::IntVar);
                m_possible_types.push_back(Type::FloatVar);
                m_possible_types.push_back(Type::DictionaryVar);
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
            if (EncodedVariableInterpreter::convert_string_to_representable_integer_var(
                        value_without_wildcards,
                        encoded_var
                )
                || EncodedVariableInterpreter::convert_string_to_representable_float_var(
                        value_without_wildcards,
                        encoded_var
                ))
            {
                converts_to_non_dict_var = true;
            }

            if (!converts_to_non_dict_var) {
                // Dictionary variable
                m_type = Type::DictionaryVar;
                m_cannot_convert_to_non_dict_var = true;
            } else {
                m_type = Type::Ambiguous;
                m_possible_types.push_back(Type::IntVar);
                m_possible_types.push_back(Type::FloatVar);
                m_possible_types.push_back(Type::DictionaryVar);
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

bool QueryToken::is_float_var() const {
    Type type;
    if (Type::Ambiguous == m_type) {
        type = m_possible_types[m_current_possible_type_ix];
    } else {
        type = m_type;
    }
    return Type::FloatVar == type;
}

bool QueryToken::is_int_var() const {
    Type type;
    if (Type::Ambiguous == m_type) {
        type = m_possible_types[m_current_possible_type_ix];
    } else {
        type = m_type;
    }
    return Type::IntVar == type;
}

bool QueryToken::is_var() const {
    Type type;
    if (Type::Ambiguous == m_type) {
        type = m_possible_types[m_current_possible_type_ix];
    } else {
        type = m_type;
    }
    return (Type::IntVar == type || Type::FloatVar == type || Type::DictionaryVar == type);
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

/**
 * Wraps the tokens returned from the log_surgeon lexer, and stores the variable ids of the tokens
 * in a search query in a set. This allows for optimized search performance.
 */
class SearchToken : public log_surgeon::Token {
public:
    std::set<int> m_type_ids_set;
};

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
bool process_var_token(
        QueryToken const& query_token,
        Archive const& archive,
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
bool find_matching_message(
        Query const& query,
        Archive& archive,
        SubQuery const*& matching_sub_query,
        File& compressed_file,
        Message& compressed_msg
);
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
SubQueryMatchabilityResult generate_logtypes_and_vars_for_subquery(
        Archive const& archive,
        string& processed_search_string,
        vector<QueryToken>& query_tokens,
        bool ignore_case,
        SubQuery& sub_query
);

bool process_var_token(
        QueryToken const& query_token,
        Archive const& archive,
        bool ignore_case,
        SubQuery& sub_query,
        string& logtype
) {
    // Even though we may have a precise variable, we still fallback to decompressing to ensure that
    // it is in the right place in the message
    sub_query.mark_wildcard_match_required();

    // Create QueryVar corresponding to token
    if (!query_token.contains_wildcards()) {
        if (EncodedVariableInterpreter::encode_and_search_dictionary(
                    query_token.get_value(),
                    archive.get_var_dictionary(),
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
            LogTypeDictionaryEntry::add_float_var(logtype);
        } else if (query_token.is_int_var()) {
            LogTypeDictionaryEntry::add_int_var(logtype);
        } else {
            LogTypeDictionaryEntry::add_dict_var(logtype);

            if (query_token.cannot_convert_to_non_dict_var()) {
                // Must be a dictionary variable, so search variable dictionary
                if (!EncodedVariableInterpreter::wildcard_search_dictionary_and_get_encoded_matches(
                            query_token.get_value(),
                            archive.get_var_dictionary(),
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

bool find_matching_message(
        Query const& query,
        Archive& archive,
        SubQuery const*& matching_sub_query,
        File& compressed_file,
        Message& compressed_msg
) {
    if (query.contains_sub_queries()) {
        matching_sub_query
                = archive.find_message_matching_query(compressed_file, query, compressed_msg);
        if (nullptr == matching_sub_query) {
            return false;
        }
    } else if ((query.get_search_begin_timestamp() > cEpochTimeMin
                || query.get_search_end_timestamp() < cEpochTimeMax))
    {
        bool found_msg = archive.find_message_in_time_range(
                compressed_file,
                query.get_search_begin_timestamp(),
                query.get_search_end_timestamp(),
                compressed_msg
        );
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

SubQueryMatchabilityResult generate_logtypes_and_vars_for_subquery(
        Archive const& archive,
        string& processed_search_string,
        vector<QueryToken>& query_tokens,
        bool ignore_case,
        SubQuery& sub_query
) {
    size_t last_token_end_pos = 0;
    string logtype;
    auto escape_handler
            = [](std::string_view constant, size_t char_to_escape_pos, string& logtype) -> void {
        auto const escape_char{enum_to_underlying_type(ir::VariablePlaceholder::Escape)};
        auto const next_char_pos{char_to_escape_pos + 1};
        // NOTE: We don't want to add additional escapes for wildcards that have been escaped. E.g.,
        // the query "\\*" should remain unchanged.
        if (next_char_pos < constant.length() && false == is_wildcard(constant[next_char_pos])) {
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
                LogTypeDictionaryEntry::add_dict_var(logtype);
                logtype += '*';
            }
        } else {
            if (!query_token.is_var()) {
                ir::append_constant_to_logtype(query_token.get_value(), escape_handler, logtype);
            } else if (!process_var_token(query_token, archive, ignore_case, sub_query, logtype)) {
                return SubQueryMatchabilityResult::WontMatch;
            }
        }
    }

    if (last_token_end_pos < processed_search_string.length()) {
        // Append from end of last token to end
        ir::append_constant_to_logtype(
                static_cast<std::string_view>(processed_search_string)
                        .substr(last_token_end_pos, string::npos),
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
    std::unordered_set<LogTypeDictionaryEntry const*> possible_logtype_entries;
    archive.get_logtype_dictionary()
            .get_entries_matching_wildcard_string(logtype, ignore_case, possible_logtype_entries);
    if (possible_logtype_entries.empty()) {
        return SubQueryMatchabilityResult::WontMatch;
    }
    sub_query.set_possible_logtypes(possible_logtype_entries);

    // Calculate the IDs of the segments that may contain results for the sub-query now that we've
    // calculated the matching logtypes and variables
    sub_query.calculate_ids_of_matching_segments();

    return SubQueryMatchabilityResult::MayMatch;
}
}  // namespace

std::optional<Query> Grep::process_raw_query(
        Archive const& archive,
        string const& search_string,
        epochtime_t search_begin_ts,
        epochtime_t search_end_ts,
        bool ignore_case,
        log_surgeon::lexers::ByteLexer& forward_lexer,
        log_surgeon::lexers::ByteLexer& reverse_lexer,
        bool use_heuristic
) {
    // Add prefix and suffix '*' to make the search a sub-string match
    string processed_search_string = "*";
    processed_search_string += search_string;
    processed_search_string += '*';
    processed_search_string = clean_up_wildcard_search_string(processed_search_string);

    // Split search_string into tokens with wildcards
    vector<QueryToken> query_tokens;
    size_t begin_pos = 0;
    size_t end_pos = 0;
    bool is_var;
    string search_string_for_sub_queries{processed_search_string};
    if (use_heuristic) {
        // Replace '?' wildcards with '*' wildcards since we currently have no support for
        // generating sub-queries with '?' wildcards. The final wildcard match on the decompressed
        // message uses the original wildcards, so correctness will be maintained.
        std::replace(
                search_string_for_sub_queries.begin(),
                search_string_for_sub_queries.end(),
                '?',
                '*'
        );
        // Clean-up in case any instances of "?*" or "*?" were changed into "**"
        search_string_for_sub_queries
                = clean_up_wildcard_search_string(search_string_for_sub_queries);
        while (get_bounds_of_next_potential_var(
                search_string_for_sub_queries,
                begin_pos,
                end_pos,
                is_var
        ))
        {
            query_tokens.emplace_back(search_string_for_sub_queries, begin_pos, end_pos, is_var);
        }
    } else {
        while (get_bounds_of_next_potential_var(
                search_string_for_sub_queries,
                begin_pos,
                end_pos,
                is_var,
                forward_lexer,
                reverse_lexer
        ))
        {
            query_tokens.emplace_back(search_string_for_sub_queries, begin_pos, end_pos, is_var);
        }
    }

    // Get pointers to all ambiguous tokens. Exclude tokens with wildcards in the middle since we
    // fall-back to decompression + wildcard matching for those.
    vector<QueryToken*> ambiguous_tokens;
    for (auto& query_token : query_tokens) {
        if (!query_token.has_greedy_wildcard_in_middle() && query_token.is_ambiguous_token()) {
            ambiguous_tokens.push_back(&query_token);
        }
    }

    // Generate a sub-query for each combination of ambiguous tokens
    // E.g., if there are two ambiguous tokens each of which could be a logtype or variable, we need
    // to create:
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
                archive,
                search_string_for_sub_queries,
                query_tokens,
                ignore_case,
                sub_query
        );
        switch (matchability) {
            case SubQueryMatchabilityResult::SupercedesAllSubQueries:
                // Since other sub-queries will be superceded by this one, we can stop processing
                // now
                return Query{
                        search_begin_ts,
                        search_end_ts,
                        ignore_case,
                        processed_search_string,
                        {}
                };
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

    return Query{
            search_begin_ts,
            search_end_ts,
            ignore_case,
            processed_search_string,
            std::move(sub_queries)
    };
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

bool Grep::get_bounds_of_next_potential_var(
        string const& value,
        size_t& begin_pos,
        size_t& end_pos,
        bool& is_var,
        log_surgeon::lexers::ByteLexer& forward_lexer,
        log_surgeon::lexers::ByteLexer& reverse_lexer
) {
    size_t const value_length = value.length();
    if (end_pos >= value_length) {
        return false;
    }

    is_var = false;
    bool contains_wildcard = false;
    while (false == is_var && false == contains_wildcard && begin_pos < value_length) {
        // Start search at end of last token
        begin_pos = end_pos;

        // Find variable begin or wildcard
        bool is_escaped = false;
        for (; begin_pos < value_length; ++begin_pos) {
            char c = value[begin_pos];

            if (is_escaped) {
                is_escaped = false;

                if (false == forward_lexer.is_delimiter(c)) {
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
                if (false == forward_lexer.is_delimiter(c)) {
                    break;
                }
            }
        }

        // Find next delimiter
        is_escaped = false;
        end_pos = begin_pos;
        for (; end_pos < value_length; ++end_pos) {
            char c = value[end_pos];

            if (is_escaped) {
                is_escaped = false;

                if (forward_lexer.is_delimiter(c)) {
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
                } else if (forward_lexer.is_delimiter(c)) {
                    // Found delimiter that's not also a wildcard
                    break;
                }
            }
        }

        if (end_pos > begin_pos) {
            bool has_prefix_wildcard = ('*' == value[begin_pos]) || ('?' == value[begin_pos]);
            bool has_suffix_wildcard = ('*' == value[end_pos - 1]) || ('?' == value[begin_pos]);
            bool has_wildcard_in_middle = false;
            for (size_t i = begin_pos + 1; i < end_pos - 1; ++i) {
                if (('*' == value[i] || '?' == value[i]) && value[i - 1] != '\\') {
                    has_wildcard_in_middle = true;
                    break;
                }
            }
            SearchToken search_token;
            if (has_wildcard_in_middle || (has_prefix_wildcard && has_suffix_wildcard)) {
                // DO NOTHING
            } else {
                StringReader string_reader;
                LogSurgeonReader reader_wrapper(string_reader);
                log_surgeon::ParserInputBuffer parser_input_buffer;
                if (has_suffix_wildcard) {  // text*
                    // TODO: creating a string reader, setting it equal to a string, to read it into
                    // the ParserInputBuffer, seems like a convoluted way to set a string equal to a
                    // string, should be improved when adding a SearchParser to log_surgeon
                    string_reader.open(value.substr(begin_pos, end_pos - begin_pos - 1));
                    parser_input_buffer.read_if_safe(reader_wrapper);
                    forward_lexer.reset();
                    forward_lexer.scan_with_wildcard(
                            parser_input_buffer,
                            value[end_pos - 1],
                            search_token
                    );
                } else if (has_prefix_wildcard) {  // *text
                    std::string value_reverse
                            = value.substr(begin_pos + 1, end_pos - begin_pos - 1);
                    std::reverse(value_reverse.begin(), value_reverse.end());
                    string_reader.open(value_reverse);
                    parser_input_buffer.read_if_safe(reader_wrapper);
                    reverse_lexer.reset();
                    reverse_lexer.scan_with_wildcard(
                            parser_input_buffer,
                            value[begin_pos],
                            search_token
                    );
                } else {  // no wildcards
                    string_reader.open(value.substr(begin_pos, end_pos - begin_pos));
                    parser_input_buffer.read_if_safe(reader_wrapper);
                    forward_lexer.reset();
                    forward_lexer.scan(parser_input_buffer, search_token);
                    search_token.m_type_ids_set.insert(search_token.m_type_ids_ptr->at(0));
                }
                // TODO: use a set so its faster
                // auto const& set = search_token.m_type_ids_set;
                // if (set.find(static_cast<int>(log_surgeon::SymbolID::TokenUncaughtStringID))
                //            == set.end()
                //     && set.find(static_cast<int>(log_surgeon::SymbolID::TokenEndID))
                //            == set.end())
                // {
                //     is_var = true;
                // }
                auto const& type = search_token.m_type_ids_ptr->at(0);
                if (type != static_cast<int>(log_surgeon::SymbolID::TokenUncaughtStringID)
                    && type != static_cast<int>(log_surgeon::SymbolID::TokenEndID))
                {
                    is_var = true;
                }
            }
        }
    }
    return (value_length != begin_pos);
}

void
Grep::calculate_sub_queries_relevant_to_file(File const& compressed_file, vector<Query>& queries) {
    for (auto& query : queries) {
        query.make_sub_queries_relevant_to_segment(compressed_file.get_segment_id());
    }
}

size_t Grep::search_and_output(
        Query const& query,
        size_t limit,
        Archive& archive,
        File& compressed_file,
        OutputFunc output_func,
        void* output_func_arg
) {
    size_t num_matches = 0;

    Message compressed_msg;
    string decompressed_msg;
    string const& orig_file_path = compressed_file.get_orig_path();
    while (num_matches < limit) {
        // Find matching message
        SubQuery const* matching_sub_query = nullptr;
        if (find_matching_message(
                    query,
                    archive,
                    matching_sub_query,
                    compressed_file,
                    compressed_msg
            )
            == false)
        {
            break;
        }

        // Decompress match
        bool decompress_successful
                = archive.decompress_message(compressed_file, compressed_msg, decompressed_msg);
        if (!decompress_successful) {
            break;
        }

        // Perform wildcard match if required
        // Check if:
        // - Sub-query requires wildcard match, or
        // - no subqueries exist and the search string is not a match-all
        if ((query.contains_sub_queries() && matching_sub_query->wildcard_match_required())
            || (query.contains_sub_queries() == false && query.search_string_matches_all() == false
            ))
        {
            bool matched = wildcard_match_unsafe(
                    decompressed_msg,
                    query.get_search_string(),
                    query.get_ignore_case() == false
            );
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

bool Grep::search_and_decompress(
        Query const& query,
        Archive& archive,
        File& compressed_file,
        Message& compressed_msg,
        string& decompressed_msg
) {
    string const& orig_file_path = compressed_file.get_orig_path();

    bool matched = false;
    while (false == matched) {
        // Find matching message
        SubQuery const* matching_sub_query = nullptr;
        bool message_found = find_matching_message(
                query,
                archive,
                matching_sub_query,
                compressed_file,
                compressed_msg
        );
        if (false == message_found) {
            return false;
        }

        // Decompress match
        bool decompress_successful
                = archive.decompress_message(compressed_file, compressed_msg, decompressed_msg);
        if (false == decompress_successful) {
            return false;
        }

        // Perform wildcard match if required
        // Check if:
        // - Sub-query requires wildcard match, or
        // - no subqueries exist and the search string is not a match-all
        if ((query.contains_sub_queries() && matching_sub_query->wildcard_match_required())
            || (query.contains_sub_queries() == false && query.search_string_matches_all() == false
            ))
        {
            matched = wildcard_match_unsafe(
                    decompressed_msg,
                    query.get_search_string(),
                    query.get_ignore_case() == false
            );
        } else {
            matched = true;
        }
    }

    return true;
}

size_t Grep::search(Query const& query, size_t limit, Archive& archive, File& compressed_file) {
    size_t num_matches = 0;

    Message compressed_msg;
    string decompressed_msg;
    string const& orig_file_path = compressed_file.get_orig_path();
    while (num_matches < limit) {
        // Find matching message
        SubQuery const* matching_sub_query = nullptr;
        if (find_matching_message(
                    query,
                    archive,
                    matching_sub_query,
                    compressed_file,
                    compressed_msg
            )
            == false)
        {
            break;
        }

        // Perform wildcard match if required
        // Check if:
        // - Sub-query requires wildcard match, or
        // - no subqueries exist and the search string is not a match-all
        if ((query.contains_sub_queries() && matching_sub_query->wildcard_match_required())
            || (query.contains_sub_queries() == false && query.search_string_matches_all() == false
            ))
        {
            // Decompress match
            bool decompress_successful
                    = archive.decompress_message(compressed_file, compressed_msg, decompressed_msg);
            if (!decompress_successful) {
                break;
            }

            bool matched = wildcard_match_unsafe(
                    decompressed_msg,
                    query.get_search_string(),
                    query.get_ignore_case() == false
            );
            if (!matched) {
                continue;
            }
        }

        ++num_matches;
    }

    return num_matches;
}
}  // namespace clp
