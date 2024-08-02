#ifndef CLP_GREP_HPP
#define CLP_GREP_HPP

#include <optional>
#include <string>
#include <variant>

#include <log_surgeon/Lexer.hpp>

#include "Defs.h"
#include "Query.hpp"
#include "streaming_archive/reader/Archive.hpp"
#include "streaming_archive/reader/File.hpp"

namespace clp {

/**
 * Represents a logtype that would match the given search query. The logtype is a sequence
 * containing values, where each value is either a static character or an integer representing
 * a variable type id. Also indicates if an integer/float variable is potentially in the dictionary
 * to handle cases containing wildcards. Note: long float and integers that cannot be encoded do not
 * fall under this case, as they are not potentially, but definitely in the dictionary, so will be
 * searched for in the dictionary regardless.
 */
class QueryLogtype {
public:
    QueryLogtype() = default;

    QueryLogtype(
            std::variant<char, int> const& val,
            std::string const& string,
            bool var_contains_wildcard
    ) {
        append_value(val, string, var_contains_wildcard);
    }

    /**
     * @param rhs
     * @return true if the current logtype is shorter than rhs, false if the current logtype
     * is longer. If equally long, true if the current logtype is lexicographically smaller than
     * rhs, false if bigger. If the logtypes are identical, true if the current search query is
     * lexicographically smaller than rhs, false if bigger. If the search queries are identical,
     * true if the first mismatch in special character locations is a non-special character for the
     * current logtype, false otherwise.
     */
    bool operator<(QueryLogtype const& rhs) const;

    /**
     * Append a logtype to the current logtype.
     * @param suffix
     */
    void append_logtype(QueryLogtype& suffix);

    /**
     * Append a single value to the current logtype.
     * @param val
     * @param string
     * @param var_contains_wildcard
     */
    void append_value(
            std::variant<char, int> const& val,
            std::string const& string,
            bool var_contains_wildcard
    );

    void set_is_encoded_with_wildcard(uint32_t i, bool value) {
        m_is_encoded_with_wildcard[i] = value;
    }

    [[nodiscard]] uint32_t get_logtype_size() const { return m_logtype.size(); }

    [[nodiscard]] std::variant<char, int> get_logtype_value(uint32_t i) const {
        return m_logtype[i];
    }

    [[nodiscard]] std::string const& get_query_string(uint32_t i) const { return m_query[i]; }

    [[nodiscard]] bool get_is_encoded_with_wildcard(uint32_t i) const {
        return m_is_encoded_with_wildcard[i];
    }

    [[nodiscard]] bool get_has_wildcard(uint32_t i) const { return m_has_wildcard[i]; }

private:
    std::vector<std::variant<char, int>> m_logtype;
    std::vector<std::string> m_query;
    std::vector<bool> m_is_encoded_with_wildcard;
    std::vector<bool> m_has_wildcard;
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
     * @param lexer DFA for determining if input is in the schema
     * @param use_heuristic
     * @return Query if it may match a message, std::nullopt otherwise
     */
    static std::optional<Query> process_raw_query(
            streaming_archive::reader::Archive const& archive,
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
     * Generates all possible logtypes that can match each substr(0,n) of the search string.
     * Requires that processed_search_string is valid, meaning that only wildcards are escaped
     * and the string does not end with an escape character.
     * @param processed_search_string
     * @param lexer
     * @return a vector of all QueryLogtypes that can match the query in processed_search_string.
     */
    static std::vector<QueryLogtype> generate_query_substring_logtypes(
            std::string& processed_search_string,
            log_surgeon::lexers::ByteLexer& lexer
    );

    /**
     * Generates the possible static-text and variable types for the given substring.
     * @param processed_search_string
     * @param begin_idx
     * @param end_idx
     * @param is_greedy_wildcard
     * @param is_non_greedy_wildcard
     * @param is_escape
     * @param lexer
     * @return a vector containing the possible substring types
     */
    static std::vector<QueryLogtype> get_possible_substr_types(
            std::string& processed_search_string,
            size_t begin_idx,
            size_t end_idx,
            std::vector<bool>& is_greedy_wildcard,
            std::vector<bool>& is_non_greedy_wildcard,
            std::vector<bool>& is_escape,
            log_surgeon::lexers::ByteLexer& lexer
    );

    /**
     * Mark the locations of non-escaped wildcards '*', '?', and escape characters '\'.
     * @param processed_search_string
     * @return a tuple containing greedy wildcard, non-greedy wildcard, and escape character
     * locations.
     */
    static std::tuple<std::vector<bool>, std::vector<bool>, std::vector<bool>>
    get_wildcard_and_escape_locations(std::string const& processed_search_string);

    /**
     * Perform DFA intersect to determine the type of variables the string can match. Also stores
     * if the string contains wildcards.
     * @param search_substr
     * @param substr_offset
     * @param is_greedy_wildcard
     * @param is_non_greedy_wildcard
     * @param is_escape
     * @param lexer
     * @return a tuple containing the set of variable types and a if the substring contains
     * wildcards.
     */
    static std::tuple<std::set<uint32_t>, bool> get_substring_variable_types(
            std::string_view search_substr,
            uint32_t substr_offset,
            std::vector<bool>& is_greedy_wildcard,
            std::vector<bool>& is_non_greedy_wildcard,
            std::vector<bool>& is_escape,
            log_surgeon::lexers::ByteLexer& lexer
    );

    /**
     * Generates the logtype string for each query logtype to compare against the logtype dictionary
     * in the archive. In this proccess, we also expand query_logtypes to contain all variations of
     * each logtype that has variables with wildcards that can be encoded. E.g. "*123" can be
     * in the segmenent as an encoded integer or in the dictionary, so both cases must be checked.
     * @param query_logtypes
     * @param lexer
     * @return A vector of query logtype strings.
     */
    static std::vector<std::string> generate_logtype_strings(
            std::vector<QueryLogtype>& query_logtypes,
            log_surgeon::lexers::ByteLexer& lexer
    );

    /**
     * Compare all possible query logtypes against the archive to determine all possible sub queries
     * that can match against messages in the archive.
     * @param query_logtypes
     * @param logtype_strings
     * @param archive
     * @param lexer
     * @param ignore_case
     * @param sub_queries
     */
    static void generate_sub_queries(
            std::vector<QueryLogtype>& query_logtypes,
            std::vector<std::string>& logtype_strings,
            streaming_archive::reader::Archive const& archive,
            log_surgeon::lexers::ByteLexer& lexer,
            bool ignore_case,
            std::vector<SubQuery>& sub_queries
    );
};
}  // namespace clp

#endif  // CLP_GREP_HPP
