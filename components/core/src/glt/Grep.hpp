#ifndef GLT_GREP_HPP
#define GLT_GREP_HPP

#include <optional>
#include <string>
#include <variant>

#include <log_surgeon/Lexer.hpp>

#include "Defs.h"
#include "Query.hpp"
#include "streaming_archive/reader/Archive.hpp"
#include "streaming_archive/reader/File.hpp"

namespace glt {
class QueryLogtype {
public:
    std::vector<std::variant<char, int>> m_logtype;
    std::vector<std::string> m_search_query;
    std::vector<bool> m_is_special;
    std::vector<bool> m_var_has_wildcard;

    auto insert (QueryLogtype& query_logtype) -> void {
        m_logtype.insert(m_logtype.end(), query_logtype.m_logtype.begin(),
                         query_logtype.m_logtype.end());
        m_search_query.insert(m_search_query.end(), query_logtype.m_search_query.begin(),
                              query_logtype.m_search_query.end());
        m_is_special.insert(m_is_special.end(), query_logtype.m_is_special.begin(),
                            query_logtype.m_is_special.end());
        m_var_has_wildcard.insert(m_var_has_wildcard.end(),
                                  query_logtype.m_var_has_wildcard.begin(),
                                  query_logtype.m_var_has_wildcard.end());
    }

    auto insert (std::variant<char, int> const& val, std::string const& string,
                 bool var_contains_wildcard) -> void {
        m_var_has_wildcard.push_back(var_contains_wildcard);
        m_logtype.push_back(val);
        m_search_query.push_back(string);
        m_is_special.push_back(false);
    }

    QueryLogtype (std::variant<char, int> const& val, std::string const& string,
                  bool var_contains_wildcard) {
        insert(val, string, var_contains_wildcard);
    }

    QueryLogtype () = default;

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

/**
 * Wraps the tokens returned from the log_surgeon lexer, and stores the variable
 * ids of the tokens in a search query in a set. This allows for optimized
 * search performance.
 */
class SearchToken : public log_surgeon::Token {
public:
    std::set<int> m_type_ids_set;
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
    typedef void (*OutputFunc)(
            std::string const& orig_file_path,
            streaming_archive::reader::Message const& compressed_msg,
            std::string const& decompressed_msg,
            void* custom_arg
    );

    // Methods
    /**
     * Processes a raw user query into a Query
     * @param archive
     * @param search_string
     * @param search_begin_ts
     * @param search_end_ts
     * @param ignore_case
     * @param forward_lexer
     * @param reverse_lexer
     * @param use_heuristic
     * @return Query if it may match a message, std::nullopt otherwise
     */
    static std::optional<Query> process_raw_query(
            streaming_archive::reader::Archive const& archive,
            std::string const& search_string,
            epochtime_t search_begin_ts,
            epochtime_t search_end_ts,
            bool ignore_case,
            log_surgeon::lexers::ByteLexer& forward_lexer,
            log_surgeon::lexers::ByteLexer& reverse_lexer,
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

    /**
     * Marks which sub-queries in each query are relevant to the given file
     * @param compressed_file
     * @param queries
     */
    static void calculate_sub_queries_relevant_to_file(
            streaming_archive::reader::File const& compressed_file,
            std::vector<Query>& queries
    );

    /**
     * Searches a file with the given query and outputs any results using the given method
     * @param query
     * @param limit
     * @param archive
     * @param compressed_file
     * @param output_func
     * @param output_func_arg
     * @return Number of matches found
     * @throw streaming_archive::reader::Archive::OperationFailed if decompression unexpectedly
     * fails
     * @throw TimestampPattern::OperationFailed if failed to insert timestamp into message
     */
    static size_t search_and_output(
            Query const& query,
            size_t limit,
            streaming_archive::reader::Archive& archive,
            streaming_archive::reader::File& compressed_file,
            OutputFunc output_func,
            void* output_func_arg
    );
    static bool search_and_decompress(
            Query const& query,
            streaming_archive::reader::Archive& archive,
            streaming_archive::reader::File& compressed_file,
            streaming_archive::reader::Message& compressed_msg,
            std::string& decompressed_msg
    );
    /**
     * Searches a file with the given query without outputting the results
     * @param query
     * @param limit
     * @param archive
     * @param compressed_file
     * @return Number of matches found
     * @throw streaming_archive::reader::Archive::OperationFailed if decompression unexpectedly
     * fails
     * @throw TimestampPattern::OperationFailed if failed to insert timestamp into message
     */
    static size_t search(
            Query const& query,
            size_t limit,
            streaming_archive::reader::Archive& archive,
            streaming_archive::reader::File& compressed_file
    );

    /**
     * Searches the segment with the given queries and outputs any results using the given method
     * This method doesn't do any column based optimizations
     * @param queries
     * @param limit
     * @param query
     * @param archive
     * @param output_func
     * @param output_func_arg
     * @return Number of matches found
     * @throw streaming_archive::reader::Archive::OperationFailed if decompression unexpectedly
     * fails
     * @throw TimestampPattern::OperationFailed if failed to insert timestamp into message
     */
    static size_t search_segment_and_output(
            std::vector<LogtypeQueries> const& queries,
            Query const& query,
            size_t limit,
            streaming_archive::reader::Archive& archive,
            OutputFunc output_func,
            void* output_func_arg
    );

    static size_t search_combined_table_and_output(
            combined_table_id_t table_id,
            std::vector<LogtypeQueries> const& queries,
            Query const& query,
            size_t limit,
            streaming_archive::reader::Archive& archive,
            OutputFunc output_func,
            void* output_func_arg
    );

    /**
     * find all messages within the segment matching the time range specified in query and output
     * those messages using the given method
     * @param query
     * @param limit
     * @param archive
     * @param output_func
     * @param output_func_arg
     * @return Number of matches found
     * @throw streaming_archive::reader::Archive::OperationFailed if decompression unexpectedly
     * fails
     * @throw TimestampPattern::OperationFailed if failed to insert timestamp into message
     */
    static size_t output_message_in_segment_within_time_range(
            Query const& query,
            size_t limit,
            streaming_archive::reader::Archive& archive,
            OutputFunc output_func,
            void* output_func_arg
    );

    static size_t output_message_in_combined_segment_within_time_range(
            Query const& query,
            size_t limit,
            streaming_archive::reader::Archive& archive,
            OutputFunc output_func,
            void* output_func_arg
    );
    /**
     * Searches the segment with the given queries and outputs any results using the given method
     * This method is optimized such that it only scans through columns that are necessary
     * @param queries
     * @param limit
     * @param query
     * @param archive
     * @param output_func
     * @param output_func_arg
     * @return Number of matches found
     * @throw streaming_archive::reader::Archive::OperationFailed if decompression unexpectedly
     * fails
     * @throw TimestampPattern::OperationFailed if failed to insert timestamp into message
     */
    static size_t search_segment_optimized_and_output(
            std::vector<LogtypeQueries> const& queries,
            Query const& query,
            size_t limit,
            streaming_archive::reader::Archive& archive,
            OutputFunc output_func,
            void* output_func_arg
    );
    /**
     * Converted a query of class Query into a set of LogtypeQueries, indexed by logtype_id
     * specifically, a Query could have n subqueries, each subquery has a fixed "vars_to_match" and
     * a set of possible logtypes. The functions converts them into a
     * logtypes->vector<vars_to_match> mapping
     *
     * @param query
     * @param segment_id
     * @return a ordered-map of list of associated LogtypeQueries indexed by logtype_id
     */
    static std::unordered_map<logtype_dictionary_id_t, LogtypeQueries>
    get_converted_logtype_query(Query const& query, size_t segment_id);

    static void get_union_of_bounds(
            std::vector<LogtypeQuery> const& sub_queries,
            size_t& var_begin_ix,
            size_t& var_end_ix
    );
};
}  // namespace glt

#endif  // GLT_GREP_HPP
