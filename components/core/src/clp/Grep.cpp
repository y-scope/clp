#include "Grep.hpp"

#include <algorithm>
#include <variant>

#include <log_surgeon/Constants.hpp>
#include <log_surgeon/Lexer.hpp>
#include <log_surgeon/Schema.hpp>
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
using log_surgeon::finite_automata::RegexDFA;
using log_surgeon::finite_automata::RegexDFAByteState;
using log_surgeon::finite_automata::RegexNFA;
using log_surgeon::finite_automata::RegexNFAByteState;
using log_surgeon::lexers::ByteLexer;
using log_surgeon::ParserAST;
using log_surgeon::SchemaAST;
using log_surgeon::SchemaVarAST;
using std::set;
using std::string;
using std::unique_ptr;
using std::variant;
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

bool QueryLogtype::operator<(QueryLogtype const& rhs) const {
    if (m_logtype.size() < rhs.m_logtype.size()) {
        return true;
    } else if (m_logtype.size() > rhs.m_logtype.size()) {
        return false;
    }
    for (uint32_t i = 0; i < m_logtype.size(); i++) {
        if (m_logtype[i] < rhs.m_logtype[i]) {
            return true;
        } else if (m_logtype[i] > rhs.m_logtype[i]) {
            return false;
        }
    }
    for (uint32_t i = 0; i < m_query.size(); i++) {
        if (m_query[i] < rhs.m_query[i]) {
            return true;
        } else if (m_query[i] > rhs.m_query[i]) {
            return false;
        }
    }
    for (uint32_t i = 0; i < m_is_encoded_with_wildcard.size(); i++) {
        if (m_is_encoded_with_wildcard[i] < rhs.m_is_encoded_with_wildcard[i]) {
            return true;
        } else if (m_is_encoded_with_wildcard[i] > rhs.m_is_encoded_with_wildcard[i]) {
            return false;
        }
    }
    return false;
}

void QueryLogtype::append_logtype(QueryLogtype& suffix) {
    m_logtype.insert(m_logtype.end(), suffix.m_logtype.begin(), suffix.m_logtype.end());
    m_query.insert(m_query.end(), suffix.m_query.begin(), suffix.m_query.end());
    m_is_encoded_with_wildcard.insert(
            m_is_encoded_with_wildcard.end(),
            suffix.m_is_encoded_with_wildcard.begin(),
            suffix.m_is_encoded_with_wildcard.end()
    );
    m_has_wildcard.insert(
            m_has_wildcard.end(),
            suffix.m_has_wildcard.begin(),
            suffix.m_has_wildcard.end()
    );
}

void QueryLogtype::append_value(
        std::variant<char, int> const& val,
        std::string const& string,
        bool var_contains_wildcard
) {
    m_has_wildcard.push_back(var_contains_wildcard);
    m_logtype.push_back(val);
    m_query.push_back(string);
    m_is_encoded_with_wildcard.push_back(false);
}

std::optional<Query> Grep::process_raw_query(
        Archive const& archive,
        string const& search_string,
        epochtime_t search_begin_ts,
        epochtime_t search_end_ts,
        bool ignore_case,
        log_surgeon::lexers::ByteLexer& lexer,
        bool use_heuristic
) {
    // Add prefix and suffix '*' to make the search a sub-string match
    string processed_search_string = "*";
    processed_search_string += search_string;
    processed_search_string += '*';
    processed_search_string = clean_up_wildcard_search_string(processed_search_string);

    vector<SubQuery> sub_queries;
    if (use_heuristic) {
        // Split search_string into tokens with wildcards
        vector<QueryToken> query_tokens;
        size_t begin_pos = 0;
        size_t end_pos = 0;
        bool is_var;
        string search_string_for_sub_queries{processed_search_string};
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
        // Get pointers to all ambiguous tokens. Exclude tokens with wildcards in the middle since
        // we fall-back to decompression + wildcard matching for those.
        vector<QueryToken*> ambiguous_tokens;
        for (auto& query_token : query_tokens) {
            if (!query_token.has_greedy_wildcard_in_middle() && query_token.is_ambiguous_token()) {
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
                    // Since other sub-queries will be superceded by this one, we can stop
                    // processing now
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
    } else {
        // Use the schema dynamic programming approach to perform the search. This iteratively
        // creates all possible logtypes that can match substring(0,n) of the query, which includes
        // all possible logtypes that can match the query itself. Then these logtypes, and their
        // corresponding variables are compared against the archive.
        static vector<set<QueryLogtype>> query_substr_logtypes(processed_search_string.size());

        // TODO: remove this when subqueries can handle '?' wildcards
        string search_string_for_sub_queries{processed_search_string};
        // Replace '?' wildcards with '*' wildcards since we currently have no support for
        // generating sub-queries with '?' wildcards. The final wildcard match on the decompressed
        // message uses the original wildcards, so correctness will be maintained.
        std::replace(
                search_string_for_sub_queries.begin(),
                search_string_for_sub_queries.end(),
                '?',
                '*'
        );

        // Get the possible logtypes for the query (but only do it once across all archives).
        static bool query_substr_logtypes_set = false;
        if (false == query_substr_logtypes_set) {
            generate_query_substring_logtypes(
                    search_string_for_sub_queries,
                    lexer,
                    query_substr_logtypes
            );
            query_substr_logtypes_set = true;
        }

        // The last entry of the query_substr_logtypes is the logtypes for the query itself. Use
        // this to determine all subqueries that may match against the current archive.
        auto& query_logtypes = query_substr_logtypes.back();
        generate_sub_queries(query_logtypes, archive, lexer, ignore_case, sub_queries);
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

void Grep::calculate_sub_queries_relevant_to_file(
        File const& compressed_file,
        vector<Query>& queries
) {
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

void Grep::generate_query_substring_logtypes(
        string& processed_search_string,
        ByteLexer& lexer,
        vector<std::set<QueryLogtype>>& query_substr_logtypes
) {
    // We need to differentiate between literal '*'/'?' and wildcards
    auto [is_greedy_wildcard, is_non_greedy_wildcard, is_escape]
            = get_wildcard_and_escape_locations(processed_search_string);

    // Consider each substr(begin_idx,end_idx) of the processed_search_string and determine if it
    // could have been compressed as static-text, a variable, or some combination of
    // variables/static-text Then we populate each entry in query_substr_logtypes which corresponds
    // to the logtype for substr(0,n). To do this, for each combination of substr(begin_idx,end_idx)
    // that reconstructs substr(0,n) (e.g., substring "*1 34", can be reconstructed from substrings
    // "*1", " ", "34"), store all possible logtypes (e.g. "*<int> <int>, "*<has#> <int>, etc.) that
    // are unique from any previously checked combination. Each entry in query_substr_logtypes is
    // used to build the following entry, with the last entry having all possible logtypes for the
    // full query itself.
    for (size_t end_idx = 1; end_idx <= processed_search_string.size(); ++end_idx) {
        // Skip strings that end with an escape character (e.g., substring " text\" from string
        // "* text\* *"). Also skip strings that end with a greedy wildcard because we are going
        // to duplicate its wildcard in the next iteration (e.g., for string "abc text* def", we
        // ignore combinations of "abc " + "text*" + " def" in favor of "abc " + "text*" + "* def"
        // as the latter will contain all logtypes capture by the former.
        if (is_escape[end_idx - 1] || is_greedy_wildcard[end_idx - 1]) {
            continue;
        }
        for (size_t begin_idx = 0; begin_idx < end_idx; ++begin_idx) {
            // Skip strings that begin with an incorrectly unescaped wildcard (e.g., substring
            // "*text" from string "* \*text *"). Also, similar to above, we ignore substrings that
            // begin with a greedy wilcard.
            if ((begin_idx > 0 && is_escape[begin_idx - 1]) || (is_greedy_wildcard[begin_idx])) {
                continue;
            }
            std::vector<QueryLogtype> possible_substr_types;
            // Don't allow an isolated wildcard to be considered a variable
            if (end_idx - 1 == begin_idx && is_greedy_wildcard[begin_idx]) {
                possible_substr_types.emplace_back('*', "*", false);
            } else if (end_idx - 1 == begin_idx && is_non_greedy_wildcard[begin_idx]) {
                possible_substr_types.emplace_back('?', "?", false);
            } else {
                set<uint32_t> variable_types;

                // If the substring is preceded or proceeded by a greedy wildcard then it's possible
                // the substring could be extended to match a var, so the wildcards are added to the
                // substring. If we don't consider this case we could miss combinations. Take for
                // example "* ab*cd *", "ab*" and "*cd" may both match a has# style variable
                // ("\w*\d+\w*"). If we decompose the string into either substrings "* " + "ab*" +
                // "cd" + " *" or "* " + "ab" + "*cd" + " *", neither would capture the possibility
                // of a logtype with the form "* <has#><has#> *", which is a valid possibility
                // during compression. Note, non-greedy wildcards do not need to be considered, for
                // example "* ab?cd *" can never match "* <has#><has#> *".
                uint32_t substr_start = begin_idx;
                uint32_t substr_end = end_idx;
                bool prev_char_is_star = begin_idx > 0 && is_greedy_wildcard[begin_idx - 1];
                bool next_char_is_star
                        = end_idx < processed_search_string.length() && is_greedy_wildcard[end_idx];
                if (prev_char_is_star) {
                    substr_start--;
                }
                if (next_char_is_star) {
                    substr_end++;
                }

                // If the substring contains a wildcard, we need to consider the case that it can
                // simultaneously match multiple variables and static text, and we need a different
                // approach to compare against the archive.
                bool contains_wildcard = false;

                // If the substring isn't surrounded by delimiters there is no reason to consider
                // the case where it is a variable as CLP would not compress it as such.

                // Preceding delimiter counts the start of log, a wildcard, or an actual delimiter.
                bool has_preceding_delimiter
                        = 0 == begin_idx || is_greedy_wildcard[begin_idx - 1]
                          || is_non_greedy_wildcard[begin_idx - 1]
                          || lexer.is_delimiter(processed_search_string[begin_idx - 1]);

                // Proceeding delimiter counts the end of log, a wildcard, or an actual delimiter.
                // However, we have to be careful about a proceeding escape character. First, if '\'
                // is a delimiter, we avoid counting the escape character. Second, if a literal '*'
                // or '?' is a delimiter, then it will appear after the escape character.
                bool has_proceeding_delimiter
                        = processed_search_string.size() == end_idx || is_greedy_wildcard[end_idx]
                          || is_non_greedy_wildcard[end_idx]
                          || (false == is_escape[end_idx]
                              && lexer.is_delimiter(processed_search_string[end_idx]))
                          || (is_escape[end_idx]
                              && lexer.is_delimiter(processed_search_string[end_idx + 1]));
                if (has_preceding_delimiter && has_proceeding_delimiter) {
                    get_substring_variable_types(
                            substr_start,
                            substr_end,
                            processed_search_string,
                            is_greedy_wildcard,
                            is_non_greedy_wildcard,
                            is_escape,
                            lexer,
                            contains_wildcard,
                            variable_types
                    );
                    bool already_added_var = false;
                    // Use the variable types to determine the possible_substr_types
                    for (int id : variable_types) {
                        auto& schema_type = lexer.m_id_symbol[id];
                        if (schema_type != "int" && schema_type != "float") {
                            if (already_added_var) {
                                continue;
                            }
                            already_added_var = true;
                        }

                        // If the substring had preceding or proceeding greedy wildcards, even when
                        // it may match a variable, it may match more. So we want to store it as
                        // "*<var>"/"<var>*"/"*<var>*" instead of just <var>. We don't need to do
                        // this if the wildcard was borrowed from the neighboring substring, as the
                        // neighboring substring will handle these cases for us.
                        bool start_star
                                = is_greedy_wildcard[substr_start] && false == prev_char_is_star;
                        bool end_star
                                = is_greedy_wildcard[substr_end - 1] && false == next_char_is_star;
                        possible_substr_types.emplace_back();
                        QueryLogtype& suffix = possible_substr_types.back();
                        if (start_star) {
                            suffix.append_value('*', "*", false);
                        }
                        suffix.append_value(
                                id,
                                processed_search_string
                                        .substr(substr_start, substr_end - substr_start),
                                contains_wildcard
                        );
                        if (end_star) {
                            suffix.append_value('*', "*", false);
                        }

                        // If the substring has no wildcards, we can safely exclude lower priority
                        // variable types.
                        if (false == contains_wildcard) {
                            break;
                        }
                    }
                }
                // If the substring matches no variables, or has a wildcard, it is potentially
                // static-text.
                if (variable_types.empty() || contains_wildcard) {
                    possible_substr_types.emplace_back();
                    auto& possible_substr_type = possible_substr_types.back();
                    for (uint32_t idx = begin_idx; idx < end_idx; idx++) {
                        char const& c = processed_search_string[idx];
                        std::string char_string({c});
                        possible_substr_type.append_value(c, char_string, false);
                    }
                }
            }

            // Use the completed set of variable types for each substr(begin_idx,end_idx) to
            // construct all possible logtypes for each substr(0,n), for all n.
            if (begin_idx > 0) {
                // Handle the case where substr(0,n) is composed of multiple
                // substr(begin_idx,end_idx).
                for (auto const& prefix : query_substr_logtypes[begin_idx - 1]) {
                    for (auto& suffix : possible_substr_types) {
                        QueryLogtype query_logtype = prefix;
                        query_logtype.append_logtype(suffix);
                        query_substr_logtypes[end_idx - 1].insert(query_logtype);
                    }
                }
            } else {
                // Handle the case where substr(0,n) == substr(begin_idx,end_idx).
                for (auto& possible_substr_type : possible_substr_types) {
                    query_substr_logtypes[end_idx - 1].insert(possible_substr_type);
                }
            }
        }
    }
}

std::tuple<std::vector<bool>, std::vector<bool>, std::vector<bool>>
Grep::get_wildcard_and_escape_locations(std::string const& processed_search_string) {
    std::vector<bool> is_greedy_wildcard;
    std::vector<bool> is_non_greedy_wildcard;
    std::vector<bool> is_escape;
    is_greedy_wildcard.reserve(processed_search_string.size());
    is_non_greedy_wildcard.reserve(processed_search_string.size());
    is_escape.reserve(processed_search_string.size());
    bool is_escaped = false;
    for (auto c : processed_search_string) {
        if (is_escaped) {
            is_greedy_wildcard.push_back(false);
            is_non_greedy_wildcard.push_back(false);
            is_escape.push_back(false);
            is_escaped = false;
        } else {
            if ('\\' == c) {
                is_escaped = true;
                is_greedy_wildcard.push_back(false);
                is_non_greedy_wildcard.push_back(false);
                is_escape.push_back(true);
            } else if ('*' == c) {
                is_greedy_wildcard.push_back(true);
                is_non_greedy_wildcard.push_back(false);
                is_escape.push_back(false);
            } else if ('?' == c) {
                is_greedy_wildcard.push_back(false);
                is_non_greedy_wildcard.push_back(true);
                is_escape.push_back(false);
            } else {
                is_greedy_wildcard.push_back(false);
                is_non_greedy_wildcard.push_back(false);
                is_escape.push_back(false);
            }
        }
    }
    return {std::move(is_greedy_wildcard), std::move(is_non_greedy_wildcard), std::move(is_escape)};
}

void Grep::get_substring_variable_types(
        uint32_t substr_start,
        uint32_t substr_end,
        std::string& schema_search_string,
        std::vector<bool>& is_greedy_wildcard,
        std::vector<bool>& is_non_greedy_wildcard,
        std::vector<bool>& is_escape,
        ByteLexer& lexer,
        bool& contains_wildcard,
        set<uint32_t>& variable_types
) {
    // To determine if a substring could be a variable we convert it to regex,
    // generate the NFA and DFA for the regex, and intersect the substring DFA with
    // the compression DFA.
    std::string regex_search_string;
    for (uint32_t idx = substr_start; idx < substr_end; idx++) {
        if (is_escape[idx]) {
            continue;
        }
        auto const& c = schema_search_string[idx];
        if (is_greedy_wildcard[idx]) {
            contains_wildcard = true;
            regex_search_string += ".*";
        } else if (is_non_greedy_wildcard[idx]) {
            contains_wildcard = true;
            regex_search_string += ".";
        } else if (log_surgeon::SchemaParser::get_special_regex_characters().contains(c)) {
            regex_search_string += "\\";
            regex_search_string += c;
        } else {
            regex_search_string += c;
        }
    }

    // Generated substring NFA from regex.
    log_surgeon::Schema substring_schema;
    // TODO: LogSurgeon should handle resetting this value.
    log_surgeon::NonTerminal::m_next_children_start = 0;
    // TODO: could use a forward/reverse lexer in place of intersect a lot of cases.
    // TODO: NFA creation not optimized at all.
    substring_schema.add_variable("search", regex_search_string, -1);
    RegexNFA<RegexNFAByteState> nfa;
    std::unique_ptr<SchemaAST> schema_ast = substring_schema.release_schema_ast_ptr();
    for (std::unique_ptr<ParserAST> const& parser_ast : schema_ast->m_schema_vars) {
        auto* schema_var_ast = dynamic_cast<SchemaVarAST*>(parser_ast.get());
        ByteLexer::Rule rule(0, std::move(schema_var_ast->m_regex_ptr));
        rule.add_ast(&nfa);
    }

    // Generate substring DFA from NFA.
    // TODO: log-surgeon needs to be refactored to allow direct usage of DFA/NFA.
    // TODO: DFA creation isn't optimized at all.
    auto const search_string_dfa = ByteLexer::nfa_to_dfa(nfa);
    auto const& schema_dfa = lexer.get_dfa();

    // Get variable types in the intersection of substring and compression DFAs.
    variable_types = schema_dfa->get_intersect(search_string_dfa);
}

void Grep::generate_sub_queries(
        set<QueryLogtype>& query_logtypes,
        Archive const& archive,
        ByteLexer& lexer,
        bool ignore_case,
        vector<SubQuery>& sub_queries
) {
    while (false == query_logtypes.empty()) {
        // Note: you need to keep the node handle to avoid deleting the object.
        auto query_logtype_nh = query_logtypes.extract(query_logtypes.begin());
        auto const& query_logtype = query_logtype_nh.value();

        // Convert each query logtype into a set of logtype strings. Logtype strings are used in the
        // sub query as they have the correct format for comparing against the archive. Also, a
        // single query logtype might represent multiple logtype strings. While static text converts
        // one-to-one, wildcard variables that may be encoded have different logtype strings when
        // comparing against the dictionary than they do when comparing against the segment.
        std::string logtype_string;
        bool has_vars = true;
        for (uint32_t i = 0; i < query_logtype.get_logtype_size(); i++) {
            auto const logtype_value = query_logtype.get_logtype_value(i);
            auto const& raw_string = query_logtype.get_query_string(i);
            auto const is_encoded_with_wildcard = query_logtype.get_is_encoded_with_wildcard(i);
            auto const has_wildcard = query_logtype.get_has_wildcard(i);
            if (std::holds_alternative<char>(logtype_value)) {
                logtype_string.push_back(std::get<char>(logtype_value));
            } else {
                auto& schema_type = lexer.m_id_symbol[std::get<int>(logtype_value)];
                encoded_variable_t encoded_var;

                // If this logtype contains wildcard variables that are being compared against the
                // dictionary, create a duplicate logtype that will compare against segment if the
                // variable may be encoded there instead.
                if (false == is_encoded_with_wildcard && has_wildcard
                    && ("int" == schema_type || "float" == schema_type))
                {
                    auto new_query_logtype = query_logtype;
                    new_query_logtype.set_is_encoded_with_wildcard(i, true);
                    query_logtypes.insert(new_query_logtype);
                }
                if (is_encoded_with_wildcard) {
                    if ("int" == schema_type) {
                        LogTypeDictionaryEntry::add_int_var(logtype_string);
                    } else if ("float" == schema_type) {
                        LogTypeDictionaryEntry::add_float_var(logtype_string);
                    }
                } else if (false == has_wildcard && "int" == schema_type
                           && EncodedVariableInterpreter::
                                   convert_string_to_representable_integer_var(
                                           raw_string,
                                           encoded_var
                                   ))
                {
                    LogTypeDictionaryEntry::add_int_var(logtype_string);
                } else if (false == has_wildcard && "float" == schema_type
                           && EncodedVariableInterpreter::convert_string_to_representable_float_var(
                                   raw_string,
                                   encoded_var
                           ))
                {
                    LogTypeDictionaryEntry::add_float_var(logtype_string);
                } else {
                    LogTypeDictionaryEntry::add_dict_var(logtype_string);
                }
            }
        }

        // Check if the logtype string exists in the logtype dictionary. If not, then this logtype
        // string does not form a useful sub query.
        std::unordered_set<LogTypeDictionaryEntry const*> possible_logtype_entries;
        archive.get_logtype_dictionary().get_entries_matching_wildcard_string(
                logtype_string,
                ignore_case,
                possible_logtype_entries
        );
        if (possible_logtype_entries.empty()) {
            continue;
        }

        // Check if the variables associated with the logtype string exist in the variable
        // dictionary. If not, then this does not form a useful sub query. If the variable is
        // encoded in the segment, we just assume it exists in the segment, as we estimate that
        // checking is slower than decompressing.
        SubQuery sub_query;
        for (uint32_t i = 0; i < query_logtype.get_logtype_size(); i++) {
            auto const logtype_value = query_logtype.get_logtype_value(i);
            auto const& raw_string = query_logtype.get_query_string(i);
            auto const is_encoded_with_wildcard = query_logtype.get_is_encoded_with_wildcard(i);
            auto const var_has_wildcard = query_logtype.get_has_wildcard(i);
            if (std::holds_alternative<int>(logtype_value)) {
                auto& schema_type = lexer.m_id_symbol[std::get<int>(logtype_value)];
                encoded_variable_t encoded_var;
                if (is_encoded_with_wildcard) {
                    sub_query.mark_wildcard_match_required();
                } else if (schema_type == "int"
                           && EncodedVariableInterpreter::
                                   convert_string_to_representable_integer_var(
                                           raw_string,
                                           encoded_var
                                   ))
                {
                    sub_query.add_non_dict_var(encoded_var);
                } else if (schema_type == "float"
                           && EncodedVariableInterpreter::convert_string_to_representable_float_var(
                                   raw_string,
                                   encoded_var
                           ))
                {
                    sub_query.add_non_dict_var(encoded_var);
                } else {
                    auto& var_dict = archive.get_var_dictionary();
                    if (var_has_wildcard) {
                        // Find matches
                        std::unordered_set<VariableDictionaryEntry const*> var_dict_entries;
                        var_dict.get_entries_matching_wildcard_string(
                                raw_string,
                                ignore_case,
                                var_dict_entries
                        );
                        if (var_dict_entries.empty()) {
                            // Not in dictionary
                            has_vars = false;
                        } else {
                            // Encode matches
                            std::unordered_set<encoded_variable_t> encoded_vars;
                            for (auto entry : var_dict_entries) {
                                encoded_vars.insert(EncodedVariableInterpreter::encode_var_dict_id(
                                        entry->get_id()
                                ));
                            }
                            sub_query.add_imprecise_dict_var(encoded_vars, var_dict_entries);
                        }
                    } else {
                        auto entry = var_dict.get_entry_matching_value(raw_string, ignore_case);
                        if (nullptr == entry) {
                            // Not in dictionary
                            has_vars = false;
                        } else {
                            encoded_variable_t encoded_var
                                    = EncodedVariableInterpreter::encode_var_dict_id(entry->get_id()
                                    );
                            sub_query.add_dict_var(encoded_var, entry);
                        }
                    }
                }
            }
        }
        if (false == has_vars) {
            continue;
        }
        if (false == possible_logtype_entries.empty()) {
            sub_query.set_possible_logtypes(possible_logtype_entries);

            // Calculate the IDs of the segments that may contain results for the sub-query now
            // that we've calculated the matching logtypes and variables
            sub_query.calculate_ids_of_matching_segments();
            sub_queries.push_back(std::move(sub_query));
        }
    }
}
}  // namespace clp
