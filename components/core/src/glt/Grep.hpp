#ifndef GLT_GREP_HPP
#define GLT_GREP_HPP

#include <optional>
#include <string>

#include <log_surgeon/Lexer.hpp>

#include "Defs.h"
#include "Query.hpp"
#include "streaming_archive/reader/Archive.hpp"
#include "streaming_archive/reader/File.hpp"

namespace glt {
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
     * @param forward_lexer DFA for determining if input is in the schema
     * @param reverse_lexer DFA for determining if reverse of input is in the schema
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
     * Returns bounds of next potential variable (either a definite variable or a token with
     * wildcards)
     * @param value String containing token
     * @param begin_pos Begin position of last token, changes to begin position of next token
     * @param end_pos End position of last token, changes to end position of next token
     * @param is_var Whether the token is definitely a variable
     * @param forward_lexer DFA for determining if input is in the schema
     * @param reverse_lexer DFA for determining if reverse of input is in the schema
     * @return true if another potential variable was found, false otherwise
     */
    static bool get_bounds_of_next_potential_var(
            std::string const& value,
            size_t& begin_pos,
            size_t& end_pos,
            bool& is_var,
            log_surgeon::lexers::ByteLexer& forward_lexer,
            log_surgeon::lexers::ByteLexer& reverse_lexer
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
     * @throw streaming_archive::reader::Archive::OperationFailed if decompression unexpectedly fails
     * @throw TimestampPattern::OperationFailed if failed to insert timestamp into message
     */
    static size_t search_segment_all_columns_and_output (
            const std::vector<LogtypeQueries>& queries,
            const Query& query,
            size_t limit,
            streaming_archive::reader::Archive& archive,
            OutputFunc output_func,
            void* output_func_arg
    );

    static size_t search_combined_table_and_output (
            combined_table_id_t table_id,
            const std::vector<LogtypeQueries>& queries,
            const Query& query,
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
     * @throw streaming_archive::reader::Archive::OperationFailed if decompression unexpectedly fails
     * @throw TimestampPattern::OperationFailed if failed to insert timestamp into message
     */
    static size_t output_message_in_segment_within_time_range (
            const Query& query,
            size_t limit,
            streaming_archive::reader::Archive& archive,
            OutputFunc output_func,
            void* output_func_arg
    );

    static size_t output_message_in_combined_segment_within_time_range (
            const Query& query,
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
     * @throw streaming_archive::reader::Archive::OperationFailed if decompression unexpectedly fails
     * @throw TimestampPattern::OperationFailed if failed to insert timestamp into message
     */
    static size_t search_segment_optimized_and_output (
            const std::vector<LogtypeQueries>& queries,
            const Query& query,
            size_t limit,
            streaming_archive::reader::Archive& archive,
            OutputFunc output_func,
            void* output_func_arg
    );
    /**
     * Converted a query of class Query into a set of LogtypeQueries, indexed by logtype_id
     * specifically, a Query could have n subqueries, each subquery has a fixed "vars_to_match" and
     * a set of possible logtypes. The functions converts them into a logtypes->vector<vars_to_match> mapping
     *
     * @param query
     * @param segment_id
     * @return a ordered-map of list of associated LogtypeQueries indexed by logtype_id
     */
    static std::unordered_map<logtype_dictionary_id_t, LogtypeQueries> get_converted_logtype_query(
            const Query& query,
            size_t segment_id
    );

    static void get_boundaries(
            const std::vector<LogtypeQuery>& sub_queries,
            size_t& left_boundary,
            size_t& right_boundary
    );
};
}  // namespace glt

#endif  // GLT_GREP_HPP
