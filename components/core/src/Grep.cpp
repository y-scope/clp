#include "Grep.hpp"

// C++ libraries
#include <algorithm>
#include <variant>

// Log surgeon
#include <log_surgeon/Constants.hpp>
#include <log_surgeon/Lexer.hpp>
#include <log_surgeon/Schema.hpp>

// Project headers
#include "EncodedVariableInterpreter.hpp"
#include "QueryToken.hpp"
#include "ir/parsing.hpp"
#include "StringReader.hpp"
#include "Utils.hpp"

using ir::is_delim;
using log_surgeon::finite_automata::RegexDFA;
using log_surgeon::finite_automata::RegexDFAByteState;
using log_surgeon::finite_automata::RegexNFA;
using log_surgeon::finite_automata::RegexNFAByteState;
using log_surgeon::lexers::ByteLexer;
using log_surgeon::ParserAST;
using log_surgeon::SchemaVarAST;
using std::set;
using std::string;
using std::unique_ptr;
using std::variant;
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

// Local prototypes
/**
 * Process a QueryToken that is definitely a variable
 * @param query_token
 * @param archive
 * @param ignore_case
 * @param sub_query
 * @param logtype
 * @param use_heuristic
 * @return true if this token might match a message, false otherwise
 */
static bool process_var_token (const QueryToken& query_token,
                               const Archive& archive,
                               bool ignore_case,
                               SubQuery& sub_query,
                               string& logtype,
                               bool use_heuristic);
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
 * @param use_heuristic
 * @return SubQueryMatchabilityResult::SupercedesAllSubQueries
 * @return SubQueryMatchabilityResult::WontMatch
 * @return SubQueryMatchabilityResult::MayMatch
 */
static SubQueryMatchabilityResult
generate_logtypes_and_vars_for_subquery (const Archive& archive, string& processed_search_string,
                                         vector<QueryToken>& query_tokens, bool ignore_case,
                                         SubQuery& sub_query, bool use_heuristic);

static bool process_var_token (const QueryToken& query_token, const Archive& archive,
                               bool ignore_case, SubQuery& sub_query, string& logtype) {
    // Even though we may have a precise variable, we still fallback to
    // decompressing to ensure that it is in the right place in the message
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

        if (query_token.is_float_var()) {
            LogTypeDictionaryEntry::add_float_var(logtype);
        } else if (query_token.is_int_var()) {
            LogTypeDictionaryEntry::add_int_var(logtype);
        } else {
            LogTypeDictionaryEntry::add_dict_var(logtype);

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

SubQueryMatchabilityResult
generate_logtypes_and_vars_for_subquery (const Archive& archive, string& processed_search_string,
                                         vector<QueryToken>& query_tokens, bool ignore_case,
                                         SubQuery& sub_query, bool use_heuristic)
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
                LogTypeDictionaryEntry::add_dict_var(logtype);
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
    // std::cout << logtype << std::endl;
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

bool Grep::process_raw_query (const Archive& archive, const string& search_string,
                              epochtime_t search_begin_ts, epochtime_t search_end_ts,
                              bool ignore_case,
                              Query& query, log_surgeon::lexers::ByteLexer& forward_lexer,
                              log_surgeon::lexers::ByteLexer& reverse_lexer,
                              bool use_heuristic)
{
    // Set properties which require no processing
    query.set_search_begin_timestamp(search_begin_ts);
    query.set_search_end_timestamp(search_end_ts);
    query.set_ignore_case(ignore_case);

    // Add prefix and suffix '*' to make the search a sub-string match
    string processed_search_string = "*";
    processed_search_string += search_string;
    processed_search_string += '*';

    // Clean-up search string
    processed_search_string = clean_up_wildcard_search_string(processed_search_string);
    query.set_search_string(processed_search_string);

    // Split search_string into tokens with wildcards
    vector<QueryToken> query_tokens;
    size_t begin_pos = 0;
    size_t end_pos = 0;
    bool is_var;
    if (use_heuristic) {
        // Replace non-greedy wildcards with greedy wildcards since we currently
        // have no support for searching compressed files with non-greedy
        // wildcards
        std::replace(processed_search_string.begin(), processed_search_string.end(), '?', '*');
        // Clean-up in case any instances of "?*" or "*?" were changed into "**"
        processed_search_string = clean_up_wildcard_search_string(processed_search_string);
        while (get_bounds_of_next_potential_var(processed_search_string, begin_pos, end_pos, is_var)) {
            query_tokens.emplace_back(processed_search_string, begin_pos, end_pos, is_var);
        }
        // Get pointers to all ambiguous tokens. Exclude tokens with wildcards in
        // the middle since we fall back to decompression + wildcard matching for
        // those.
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
            auto matchability = generate_logtypes_and_vars_for_subquery(archive,
                                                                        processed_search_string,
                                                                        query_tokens,
                                                                        query.get_ignore_case(),
                                                                        sub_query,
                                                                        use_heuristic);
            switch (matchability) {
                case SubQueryMatchabilityResult::SupercedesAllSubQueries:
                    // Clear all sub-queries since they will be superseded by this
                    // sub-query
                    query.clear_sub_queries();

                    // Since other sub-queries will be superseded by this one, we
                    // can stop processing now
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
    } else {
        // DFA search
        vector<set<QueryLogtype>> query_matrix(processed_search_string.size());
        for (uint32_t i = 0; i < processed_search_string.size(); i++) {
            for (uint32_t j = 0; j <= i; j++) {
                std::string current_string = processed_search_string.substr(j, i - j + 1);
                std::vector<QueryLogtype> suffixes;
                SearchToken search_token;
                if (current_string == "*") {
                    suffixes.emplace_back('*', current_string);
                } else {
                    // TODO: add this step to the documentation
                    // add * if preceding and proceeding characters are *
                    bool prev_star = j > 0 && processed_search_string[j - 1] == '*';
                    bool next_star = i < processed_search_string.back() - 1 &&
                                     processed_search_string[i + 1] == '*';
                    if (prev_star) {
                        current_string.insert(0, "*");
                    }
                    if (next_star) {
                        current_string.push_back('*');
                    }
                    StringReader string_reader;
                    log_surgeon::ParserInputBuffer parser_input_buffer;
                    ReaderInterfaceWrapper reader_wrapper(string_reader);
                    std::string regex_search_string;
                    bool contains_wildcard = false;
                    for (char const& c : current_string) {
                        if (c == '*') {
                            contains_wildcard = true;
                            regex_search_string.push_back('.');
                        } else if (
                                log_surgeon::SchemaParser::get_special_regex_characters().find(c) !=
                                log_surgeon::SchemaParser::get_special_regex_characters().end()) {
                            regex_search_string.push_back('\\');
                        }
                        regex_search_string.push_back(c);
                    }
                    log_surgeon::NonTerminal::m_next_children_start = 0;
                    log_surgeon::Schema schema2;
                    schema2.add_variable("search", regex_search_string, -1);
                    RegexNFA<RegexNFAByteState> nfa;
                    for (std::unique_ptr<ParserAST> const& parser_ast : schema2.get_schema_ast_ptr()->m_schema_vars) {
                        auto* schema_var_ast = dynamic_cast<SchemaVarAST*>(parser_ast.get());
                        ByteLexer::Rule rule(0, std::move(schema_var_ast->m_regex_ptr));
                        rule.add_ast(&nfa);
                    }
                    // TODO: this is obviously bad, but the code needs to be reorganized a lot
                    // to fix the fact that DFAs and NFAs can't be used without a lexer
                    unique_ptr<RegexDFA<RegexDFAByteState>> dfa2 = forward_lexer.nfa_to_dfa(nfa);
                    unique_ptr<RegexDFA<RegexDFAByteState>> const& dfa1 = forward_lexer.get_dfa();
                    set<uint32_t> schema_types = dfa1->get_intersect(dfa2);
                    bool is_surrounded_by_delims = false;
                    if ((j == 0 || current_string[0] == '*' || 
                         forward_lexer.is_delimiter(processed_search_string[j - 1])) &&
                        (i == processed_search_string.size() - 1 ||
                         current_string.back() == '*' || 
                         forward_lexer.is_delimiter(processed_search_string[i + 1]))) {
                        is_surrounded_by_delims = true;
                    }
                    if (is_surrounded_by_delims) {
                        for (int id : schema_types) {
                            bool start_star = current_string[0] == '*' && false == prev_star;
                            bool end_star = current_string.back() == '*' && false == next_star;
                            if ( start_star && end_star) {
                                suffixes.emplace_back('*', "*");
                                QueryLogtype& suffix = suffixes.back();
                                suffix.insert(id, current_string);
                                suffix.insert('*', "*");
                            } else if (start_star) {
                                suffixes.emplace_back('*', "*");
                                QueryLogtype& suffix = suffixes.back();
                                suffix.insert(id, current_string);
                            } else if (end_star) {
                                suffixes.emplace_back(id, current_string);
                                QueryLogtype& suffix = suffixes.back();
                                suffix.insert('*', "*");
                            } else {
                                suffixes.emplace_back(id, current_string);
                            }
                            if (false == contains_wildcard) {
                                // we only want the highest prio type if no wildcard
                                break;
                            }
                        }
                    }
                    if (schema_types.empty() || contains_wildcard ||
                        is_surrounded_by_delims == false) {
                        suffixes.emplace_back();
                        auto& suffix = suffixes.back();
                        uint32_t start_id = prev_star ? 1 : 0;
                        uint32_t end_id = next_star ? current_string.size() - 1 :
                                          current_string.size();
                        for(uint32_t k = start_id; k < end_id; k++) {
                            char const& c = current_string[k];
                            std::string char_string({c});
                            suffix.insert(c, char_string);
                        }
                    }
                }
                set<QueryLogtype>& new_queries = query_matrix[i];
                if (j > 0) {
                    for (QueryLogtype const& prefix : query_matrix[j - 1]) {
                        for (QueryLogtype& suffix : suffixes) {
                            QueryLogtype new_query = prefix;
                            new_query.insert(suffix);
                            new_queries.insert(new_query);
                        }
                    }
                } else {
                    // handles first column
                    for (QueryLogtype& suffix : suffixes) {
                        new_queries.insert(suffix);
                    }
                }
            }
        }
        uint32_t last_row = query_matrix.size() - 1;
        /*
        std::cout << "query_matrix" << std::endl;
        for(set<QueryLogtype>& query_logtypes : query_matrix) {
            for(QueryLogtype const& query_logtype : query_logtypes) {
                for(uint32_t i = 0; i < query_logtype.m_logtype.size(); i++) {
                    auto& val = query_logtype.m_logtype[i];
                    auto& str = query_logtype.m_search_query[i];
                    if (std::holds_alternative<char>(val)) {
                        std::cout << std::get<char>(val);
                    } else {
                        std::cout << "<" << forward_lexer.m_id_symbol[std::get<int>(val)] << ">";
                        std::cout << "(" << str << ")";
                    }
                }
                std::cout << " | ";
            }
            std::cout << std::endl;
        }
        std::cout << query_matrix[last_row].size() << std::endl;
        */
        for (QueryLogtype const& query_logtype: query_matrix[last_row]) {
            SubQuery sub_query;
            std::string logtype_string;
            bool has_vars = true;
            bool has_special = false;
            for(uint32_t i = 0; i < query_logtype.m_logtype.size(); i++) {
                auto const& value = query_logtype.m_logtype[i];
                auto const& var_str = query_logtype.m_search_query[i];
                auto const& is_special = query_logtype.m_is_special[i];
                if (std::holds_alternative<char>(value)) {
                    logtype_string.push_back(std::get<char>(value));
                } else {
                    auto& schema_type = forward_lexer.m_id_symbol[std::get<int>(value)];
                    encoded_variable_t encoded_var;
                    // Create a duplicate query that will treat a wildcard
                    // int/float as an int/float
                    // TODO: this is wrong you don't care if query has a wildcard, just that var.
                    //       also all queries have wildcard so this variable seems useless
                    if(false == is_special && query_logtype.m_has_wildcard && (schema_type == "int" ||schema_type == "float")) {
                        QueryLogtype new_query_logtype = query_logtype;
                        new_query_logtype.m_is_special[i] = true;
                        // TODO: this is kinda sketchy, but it'll work because 
                        // of how the < operator is defined
                        query_matrix[last_row].insert(new_query_logtype);
                    }
                    if (is_special) {
                        sub_query.mark_wildcard_match_required();
                        if (schema_type == "int") {
                            LogTypeDictionaryEntry::add_int_var(logtype_string);
                        } else if (schema_type == "float") {
                            LogTypeDictionaryEntry::add_float_var(logtype_string);
                        }
                    } else if( schema_type == "int" && EncodedVariableInterpreter::convert_string_to_representable_integer_var(var_str, encoded_var)) {
                        LogTypeDictionaryEntry::add_int_var(logtype_string);
                        sub_query.add_non_dict_var(encoded_var);
                    } else if (schema_type == "float" && EncodedVariableInterpreter::convert_string_to_representable_float_var(var_str, encoded_var)) {
                        LogTypeDictionaryEntry::add_float_var(logtype_string);
                        sub_query.add_non_dict_var(encoded_var);
                    } else {
                        LogTypeDictionaryEntry::add_dict_var(logtype_string);
                        auto& var_dict = archive.get_var_dictionary();
                        if(query_logtype.m_has_wildcard) {
                            // Find matches
                            std::unordered_set<const VariableDictionaryEntry*> var_dict_entries;
                            var_dict.get_entries_matching_wildcard_string(var_str, ignore_case, var_dict_entries);
                            if (var_dict_entries.empty()) {
                                // Not in dictionary
                                has_vars = false;
                            } else {
                                // Encode matches
                                std::unordered_set<encoded_variable_t> encoded_vars;
                                for (auto entry : var_dict_entries) {
                                    encoded_vars.insert(
                                            EncodedVariableInterpreter::encode_var_dict_id(
                                                    entry->get_id()));
                                }
                                sub_query.add_imprecise_dict_var(encoded_vars, var_dict_entries);
                            }
                        } else {
                            auto entry = var_dict.get_entry_matching_value(
                                    var_str, ignore_case);
                            if (nullptr == entry) {
                                // Not in dictionary
                                has_vars = false;
                            } else {
                                encoded_variable_t encoded_var = EncodedVariableInterpreter::encode_var_dict_id(
                                        entry->get_id());
                                sub_query.add_dict_var(encoded_var, entry);
                            }
                        }
                    }
                }
            }
            if(false == has_vars) {
                continue;
            }
            std::unordered_set<const LogTypeDictionaryEntry*> possible_logtype_entries;
            archive.get_logtype_dictionary().get_entries_matching_wildcard_string(logtype_string, ignore_case,
                                                                                  possible_logtype_entries);
            if (false == possible_logtype_entries.empty()) {
                //std::cout << logtype_string << std::endl;
                sub_query.set_possible_logtypes(possible_logtype_entries);

                // Calculate the IDs of the segments that may contain results for the sub-query now that we've calculated the matching logtypes and variables
                sub_query.calculate_ids_of_matching_segments();
                query.add_sub_query(sub_query);
            }
        }
    }
    //std::cout << query.get_sub_queries().size() << std::endl;
    //for (auto const& sub_query : query.get_sub_queries()) {
    //    sub_query.print();
    //}
    return query.contains_sub_queries();
}

bool Grep::get_bounds_of_next_potential_var (const string& value, size_t& begin_pos,
                                             size_t& end_pos, bool& is_var) {
    const auto value_length = value.length();
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
                    // Found escaped non-delimiter, so reverse the index to retain the escape character
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

            if (is_decimal_digit(c)) {
                contains_decimal_digit = true;
            } else if (is_alphabet(c)) {
                contains_alphabet = true;
            }
        }

        // Treat token as a definite variable if:
        // - it contains a decimal digit, or
        // - it could be a multi-digit hex value, or
        // - it's directly preceded by an equals sign and contains an alphabet without a wildcard between the equals sign and the first alphabet of the token
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

bool Grep::get_bounds_of_next_potential_var (const string& value, size_t& begin_pos,
                                 size_t& end_pos, bool& is_var,
                                 log_surgeon::lexers::ByteLexer& forward_lexer,
                                 log_surgeon::lexers::ByteLexer& reverse_lexer) {

    const size_t value_length = value.length();
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

                if(false == forward_lexer.is_delimiter(c)) {
                    // Found escaped non-delimiter, so reverse the index to retain the escape character
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
            bool has_suffix_wildcard = ('*' == value[end_pos - 1]) || ('?' == value[begin_pos]);;
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
                ReaderInterfaceWrapper reader_wrapper(string_reader);
                log_surgeon::ParserInputBuffer parser_input_buffer;
                if (has_suffix_wildcard) { //text*
                    // TODO: creating a string reader, setting it equal to a 
                    //  string, to read it into the ParserInputBuffer, seems
                    //  like a convoluted way to set a string equal to a string,
                    //  should be improved when adding a SearchParser to 
                    //  log_surgeon
                    string_reader.open(value.substr(begin_pos, end_pos - begin_pos - 1));
                    parser_input_buffer.read_if_safe(reader_wrapper);
                    forward_lexer.reset();
                    forward_lexer.scan_with_wildcard(parser_input_buffer,
                                                     value[end_pos - 1],
                                                     search_token);
                } else if (has_prefix_wildcard) { // *text
                    std::string value_reverse = value.substr(begin_pos + 1,
                                                             end_pos - begin_pos - 1);
                    std::reverse(value_reverse.begin(), value_reverse.end());
                    string_reader.open(value_reverse);
                    parser_input_buffer.read_if_safe(reader_wrapper);
                    reverse_lexer.reset();
                    reverse_lexer.scan_with_wildcard(parser_input_buffer,
                                                     value[begin_pos],
                                                     search_token);
                } else { // no wildcards
                    string_reader.open(value.substr(begin_pos, end_pos - begin_pos));
                    parser_input_buffer.read_if_safe(reader_wrapper);
                    forward_lexer.reset();
                    forward_lexer.scan(parser_input_buffer, search_token);
                    search_token.m_type_ids_set.insert(search_token.m_type_ids_ptr->at(0));
                }
                // TODO: use a set so its faster
                // auto const& set = search_token.m_type_ids_set;
                // if (set.find((int) log_surgeon::SymbolID::TokenUncaughtStringID) == set.end() &&
                //     set.find((int) log_surgeon::SymbolID::TokenEndID) == set.end())
                // {
                //     is_var = true;
                // }
                auto const& type = search_token.m_type_ids_ptr->at(0);
                if (type != (int)log_surgeon::SymbolID::TokenUncaughtStringID &&
                    type != (int)log_surgeon::SymbolID::TokenEndID) {
                    is_var = true;
                }
            }
        }
    }
    return (value_length != begin_pos);
}

void Grep::calculate_sub_queries_relevant_to_file (const File& compressed_file, vector<Query>& queries) {
    for (auto& query : queries) {
        query.make_sub_queries_relevant_to_segment(compressed_file.get_segment_id());
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
            bool matched = wildcard_match_unsafe(decompressed_msg, query.get_search_string(),
                                                 query.get_ignore_case() == false);
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
            matched = wildcard_match_unsafe(decompressed_msg, query.get_search_string(),
                                            query.get_ignore_case() == false);
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

            bool matched = wildcard_match_unsafe(decompressed_msg, query.get_search_string(),
                                                 query.get_ignore_case() == false);
            if (!matched) {
                continue;
            }
        }

        ++num_matches;
    }

    return num_matches;
}
