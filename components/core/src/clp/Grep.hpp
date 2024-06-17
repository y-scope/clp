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
 * containing values, where each value is either a static character or an integers representing
 * a variable type id. Also indicates if an integer/float variable is potentially in the dictionary
 * to handle cases containing wildcards. Note: long float and integers that cannot be encoded do not
 * fall under this case, as they are not potentially, but definitely in the dictionary, so will be
 * searched for in the dictionary regardless.
 */
class QueryLogtype {
public:
    std::vector<std::variant<char, int>> m_logtype;
    std::vector<std::string> m_search_query;
    std::vector<bool> m_is_potentially_in_dict;
    std::vector<bool> m_var_has_wildcard;

    /**
     * Append a logtype to the current logtype.
     * @param suffix
     */
    auto append_logtype(QueryLogtype& suffix) -> void {
        m_logtype.insert(m_logtype.end(), suffix.m_logtype.begin(), suffix.m_logtype.end());
        m_search_query.insert(
                m_search_query.end(),
                suffix.m_search_query.begin(),
                suffix.m_search_query.end()
        );
        m_is_potentially_in_dict.insert(
                m_is_potentially_in_dict.end(),
                suffix.m_is_potentially_in_dict.begin(),
                suffix.m_is_potentially_in_dict.end()
        );
        m_var_has_wildcard.insert(
                m_var_has_wildcard.end(),
                suffix.m_var_has_wildcard.begin(),
                suffix.m_var_has_wildcard.end()
        );
    }

    /**
     * Append a single value to the current logtype.
     * @param val
     * @param string
     * @param var_contains_wildcard
     */
    auto append_value(
            std::variant<char, int> const& val,
            std::string const& string,
            bool var_contains_wildcard
    ) -> void {
        m_var_has_wildcard.push_back(var_contains_wildcard);
        m_logtype.push_back(val);
        m_search_query.push_back(string);
        m_is_potentially_in_dict.push_back(false);
    }

    QueryLogtype(
            std::variant<char, int> const& val,
            std::string const& string,
            bool var_contains_wildcard
    ) {
        append_value(val, string, var_contains_wildcard);
    }

    QueryLogtype() = default;

    /**
     * @param rhs
     * @return true if the current logtype is shorter than rhs, false if the current logtype
     * is longer. If equally long, true if the current logtype is lexicographically smaller than
     * rhs, false if bigger. If the logtypes are identical, true if the current search query is
     * lexicographically smaller than rhs, false if bigger. If the search queries are identical,
     * true if the first mismatch in special character locations is a non-special character for the
     * current logtype, false otherwise.
     */
    bool operator<(QueryLogtype const& rhs) const {
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
        for (uint32_t i = 0; i < m_search_query.size(); i++) {
            if (m_search_query[i] < rhs.m_search_query[i]) {
                return true;
            } else if (m_search_query[i] > rhs.m_search_query[i]) {
                return false;
            }
        }
        for (uint32_t i = 0; i < m_is_potentially_in_dict.size(); i++) {
            if (m_is_potentially_in_dict[i] < rhs.m_is_potentially_in_dict[i]) {
                return true;
            } else if (m_is_potentially_in_dict[i] > rhs.m_is_potentially_in_dict[i]) {
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
};
}  // namespace clp

#endif  // CLP_GREP_HPP
