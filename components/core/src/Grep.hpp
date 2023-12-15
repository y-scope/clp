#ifndef GREP_HPP
#define GREP_HPP

// C++ libraries
#include <string>
#include <variant>

// Log surgeon
#include <log_surgeon/Lexer.hpp>

// Project headers
#include "Defs.h"
#include "Query.hpp"
#include "streaming_archive/reader/Archive.hpp"
#include "streaming_archive/reader/File.hpp"

class QueryLogtype {
public:
    std::vector<std::variant<char, int>> m_logtype;
    std::vector<std::string> m_search_query;
    std::vector<bool> m_is_special;
    bool m_has_wildcard = false;

    auto insert (QueryLogtype& query_logtype) -> void {
        m_logtype.insert(m_logtype.end(), query_logtype.m_logtype.begin(), query_logtype.m_logtype.end());
        m_search_query.insert(m_search_query.end(), query_logtype.m_search_query.begin(), query_logtype.m_search_query.end());
        m_is_special.insert(m_is_special.end(), query_logtype.m_is_special.begin(), query_logtype.m_is_special.end());
        m_has_wildcard = m_has_wildcard||query_logtype.m_has_wildcard;
    }

    auto insert (std::variant<char, int> const& val, std::string const& string) -> void {
        if(std::holds_alternative<char>(val) && std::get<char>(val) == '*') {
            m_has_wildcard = true;
        }
        m_logtype.push_back(val);
        m_search_query.push_back(string);
        m_is_special.push_back(false);
    }
    
    QueryLogtype(std::variant<char, int> const& val, std::string const& string) {
        insert(val, string);
    }
    
    bool operator<(const QueryLogtype &rhs) const{
        if(m_logtype.size() < rhs.m_logtype.size()) {
            return true;
        } else if (m_logtype.size() > rhs.m_logtype.size()) {
            return false;
        }
        for(uint32_t i = 0; i < m_logtype.size(); i++) {
            if(m_logtype[i] < rhs.m_logtype[i]) {
                return true;
            } else if(m_logtype[i] > rhs.m_logtype[i]) {
                return false;
            }
        }
        for(uint32_t i = 0; i < m_search_query.size(); i++) {
            if(m_search_query[i] < rhs.m_search_query[i]) {
                return true;
            } else if(m_search_query[i] > rhs.m_search_query[i]) {
                return false;
            }
        }
        for(uint32_t i = 0; i < m_is_special.size(); i++) {
            if(m_is_special[i] < rhs.m_is_special[i]) {
                return true;
            } else if(m_is_special[i] > rhs.m_is_special[i]) {
                return false;
            }
        }
        return false;
    }
    
};

class Grep {
    
public:
    // Types
    /**
     * Handles search result
     * @param orig_file_path Path of uncompressed file
     * @param compressed_msg
     * @param decompressed_msg
     * @param custom_arg Custom argument for the output function
     */
    typedef void (*OutputFunc) (const std::string& orig_file_path,
                                const streaming_archive::reader::Message& compressed_msg,
                                const std::string& decompressed_msg, void* custom_arg);

    // Methods
    /**
     * Processes a raw user query into a Query
     * @param archive
     * @param search_string
     * @param search_begin_ts
     * @param search_end_ts
     * @param ignore_case
     * @param query
     * @param forward_lexer DFA for determining if input is in the schema
     * @param reverse_lexer DFA for determining if reverse of input is in the
     * schema
     * @param use_heuristic
     * @return true if query may match messages, false otherwise
     */
    static bool process_raw_query (const streaming_archive::reader::Archive& archive,
                                   const std::string& search_string, epochtime_t search_begin_ts,
                                   epochtime_t search_end_ts, bool ignore_case, Query& query,
                                   log_surgeon::lexers::ByteLexer& forward_lexer,
                                   log_surgeon::lexers::ByteLexer& reverse_lexer,
                                   bool use_heuristic);

    /**
     * Returns bounds of next potential variable (either a definite variable or
     * a token with wildcards)
     * @param value String containing token
     * @param begin_pos Begin position of last token, changes to begin position
     * of next token
     * @param end_pos End position of last token, changes to end position of
     * next token
     * @param is_var Whether the token is definitely a variable
     * @return true if another potential variable was found, false otherwise
     */
    static bool get_bounds_of_next_potential_var (const std::string& value, size_t& begin_pos,
                                                  size_t& end_pos, bool& is_var);

    /**
     * Returns bounds of next potential variable (either a definite variable or
     * a token with wildcards)
     * @param value String containing token
     * @param begin_pos Begin position of last token, changes to begin position
     * of next token
     * @param end_pos End position of last token, changes to end position of
     * next token
     * @param is_var Whether the token is definitely a variable
     * @param forward_lexer DFA for determining if input is in the schema
     * @param reverse_lexer DFA for determining if reverse of input is in the
     * schema
     * @param post_processed_string
     * @return true if another potential variable was found, false otherwise
     */
    static bool get_bounds_of_next_potential_var (const std::string& value, size_t& begin_pos,
                                                  size_t& end_pos, bool& is_var,
                                                  log_surgeon::lexers::ByteLexer& forward_lexer,
                                                  log_surgeon::lexers::ByteLexer& reverse_lexer);
    /**
     * Marks which sub-queries in each query are relevant to the given file
     * @param compressed_file
     * @param queries
     */
    static void
    calculate_sub_queries_relevant_to_file (const streaming_archive::reader::File& compressed_file,
                                            std::vector<Query>& queries);

    /**
     * Searches a file with the given query and outputs any results using the
     * given method
     * @param query
     * @param limit
     * @param archive
     * @param compressed_file
     * @param output_func
     * @param output_func_arg
     * @return Number of matches found
     * @throw streaming_archive::reader::Archive::OperationFailed if
     * decompression unexpectedly fails
     * @throw TimestampPattern::OperationFailed if failed to insert timestamp
     * into message
     */
    static size_t search_and_output (const Query& query, size_t limit,
                                     streaming_archive::reader::Archive& archive,
                                     streaming_archive::reader::File& compressed_file,
                                     OutputFunc output_func, void* output_func_arg);

    static bool
    search_and_decompress (const Query& query, streaming_archive::reader::Archive& archive,
                           streaming_archive::reader::File& compressed_file,
                           streaming_archive::reader::Message& compressed_msg,
                           std::string& decompressed_msg);
    /**
     * Searches a file with the given query without outputting the results
     * @param query
     * @param limit
     * @param archive
     * @param compressed_file
     * @return Number of matches found
     * @throw streaming_archive::reader::Archive::OperationFailed if
     * decompression unexpectedly fails
     * @throw TimestampPattern::OperationFailed if failed to insert timestamp
     * into message
     */
    static size_t search (const Query& query, size_t limit,
                          streaming_archive::reader::Archive& archive,
                          streaming_archive::reader::File& compressed_file);
};


/**
 * Wraps the tokens returned from the log_surgeon lexer, and stores the variable
 * ids of the tokens in a search query in a set. This allows for optimized
 * search performance.
 */
class SearchToken : public log_surgeon::Token {
public:
    std::set<int> m_type_ids_set;
};

#endif // GREP_HPP
