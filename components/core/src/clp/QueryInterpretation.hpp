#ifndef CLP_GREP_QUERY_INTERPRETATION_HPP
#define CLP_GREP_QUERY_INTERPRETATION_HPP

#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <log_surgeon/Lexer.hpp>

namespace clp {
/**
 * Stores a view into the SearchString class.
 */
class SearchStringView {
public:
    SearchStringView(
            std::vector<bool> const& is_greedy_wildcard,
            std::vector<bool> const& is_non_greedy_wildcard,
            std::vector<bool> const& is_escape,
            std::string const& processed_search_string,
            uint32_t begin_idx,
            uint32_t end_idx

    )
            : m_is_greedy_wildcard(is_greedy_wildcard),
              m_is_non_greedy_wildcard(is_non_greedy_wildcard),
              m_is_escape(is_escape),
              m_processed_search_string(processed_search_string),
              m_begin_idx(begin_idx),
              m_end_idx(end_idx) {}

    void extend_to_adjacent_wildcards();

    [[nodiscard]] bool is_greedy_wildcard() const {
        return 1 == length() && m_is_greedy_wildcard[m_begin_idx];
    }

    [[nodiscard]] bool is_non_greedy_wildcard() const {
        return 1 == length() && m_is_non_greedy_wildcard[m_begin_idx];
    }

    [[nodiscard]] bool starts_or_ends_with_wildcard() const {
        return m_is_greedy_wildcard[m_begin_idx] || m_is_greedy_wildcard[m_end_idx - 1];
    }

    [[nodiscard]] bool surrounded_by_delims(log_surgeon::lexers::ByteLexer const& lexer) const;

    [[nodiscard]] uint32_t length() const { return m_end_idx - m_begin_idx; }

    [[nodiscard]] bool get_value_is_greedy_wildcard(uint32_t const idx) const {
        return m_is_greedy_wildcard[m_begin_idx + idx];
    }

    [[nodiscard]] bool get_value_is_non_greedy_wildcard(uint32_t const idx) const {
        return m_is_non_greedy_wildcard[m_begin_idx + idx];
    }

    [[nodiscard]] bool get_value_is_escape(uint32_t const idx) const {
        return m_is_escape[m_begin_idx + idx];
    }

    [[nodiscard]] char get_value(uint32_t const idx) const {
        return m_processed_search_string[m_begin_idx + idx];
    }

    [[nodiscard]] std::string get_substr_copy() const {
        return m_processed_search_string.substr(m_begin_idx, m_end_idx - m_begin_idx);
    }

private:
    std::vector<bool> const& m_is_greedy_wildcard;
    std::vector<bool> const& m_is_non_greedy_wildcard;
    std::vector<bool> const& m_is_escape;
    std::string const& m_processed_search_string;
    uint32_t m_begin_idx;
    uint32_t m_end_idx;
};

/**
 * Stores metadata about the query.
 */
class SearchString {
public:
    explicit SearchString(std::string processed_search_string);

    [[nodiscard]] std::string substr(uint32_t const begin_idx, uint32_t const length) const {
        return m_processed_search_string.substr(begin_idx, length);
    }

    [[nodiscard]] SearchStringView
    create_view(uint32_t const start_idx, uint32_t const end_idx) const {
        return SearchStringView{
                m_is_greedy_wildcard,
                m_is_non_greedy_wildcard,
                m_is_escape,
                m_processed_search_string,
                start_idx,
                end_idx
        };
    }

    [[nodiscard]] uint32_t length() const { return m_processed_search_string.size(); }

    [[nodiscard]] bool get_value_is_escape(uint32_t const idx) const { return m_is_escape[idx]; }

private:
    // std::vector<bool> is specialized so use std::vector<char> instead
    std::vector<bool> m_is_greedy_wildcard;
    std::vector<bool> m_is_non_greedy_wildcard;
    std::vector<bool> m_is_escape;
    std::string m_processed_search_string;
};

/**
 * Represents a static substring in the query string as a token.
 */
class StaticQueryToken {
public:
    explicit StaticQueryToken(std::string query_substring)
            : m_query_substring(std::move(query_substring)) {}

    bool operator==(StaticQueryToken const& rhs) const = default;

    bool operator!=(StaticQueryToken const& rhs) const = default;

    auto operator<=>(StaticQueryToken const& rhs) const = default;

    void append(StaticQueryToken const& rhs);

    [[nodiscard]] std::string const& get_query_substring() const { return m_query_substring; }

private:
    std::string m_query_substring;
};

/**
 * Represents variable substring in the query string as a token.
 */
class VariableQueryToken {
public:
    VariableQueryToken(
            uint32_t const variable_type,
            std::string query_substring,
            bool const has_wildcard,
            bool const is_encoded
    )
            : m_variable_type(variable_type),
              m_query_substring(std::move(query_substring)),
              m_has_wildcard(has_wildcard),
              m_is_encoded(is_encoded) {}

    bool operator==(VariableQueryToken const& rhs) const = default;

    auto operator<=>(VariableQueryToken const& rhs) const = default;

    [[nodiscard]] uint32_t get_variable_type() const { return m_variable_type; }

    [[nodiscard]] std::string const& get_query_substring() const { return m_query_substring; }

    [[nodiscard]] bool get_has_wildcard() const { return m_has_wildcard; }

    [[nodiscard]] bool get_is_encoded_with_wildcard() const {
        return m_is_encoded && m_has_wildcard;
    }

private:
    uint32_t m_variable_type;
    std::string m_query_substring;
    bool m_has_wildcard{false};
    bool m_is_encoded{false};
};

/**
 * Represents a logtype that would match the given search query. The logtype is a sequence
 * containing values, where each value is either a static character or an integer representing
 * a variable type id. Also indicates if an integer/float variable is potentially in the dictionary
 * to handle cases containing wildcards. Note: long float and integers that cannot be encoded do not
 * fall under this case, as they are not potentially, but definitely in the dictionary, so will be
 * searched for in the dictionary regardless.
 */
class QueryInterpretation {
public:
    QueryInterpretation() = default;

    explicit QueryInterpretation(std::string const& query_substring) {
        append_static_token(query_substring);
    }

    QueryInterpretation(
            uint32_t const variable_type,
            std::string query_substring,
            bool const contains_wildcard,
            bool const is_encoded
    ) {
        append_variable_token(
                variable_type,
                std::move(query_substring),
                contains_wildcard,
                is_encoded
        );
    }

    /**
     * Ignores m_logtype_string.
     * @param rhs
     * @return if m_logtype is equal
     */
    bool operator==(QueryInterpretation const& rhs) const { return m_logtype == rhs.m_logtype; }

    /**
     * @param rhs
     * @return true if the current logtype is shorter than rhs, false if the current logtype
     * is longer. If equally long, true if the current logtype is lexicographically smaller than
     * rhs, false if bigger. If the logtypes are identical, true if the current search query is
     * lexicographically smaller than rhs, false if bigger. If the search queries are identical,
     * true if the first mismatch in special character locations is a non-special character for the
     * current logtype, false otherwise. Ignores m_logtype_string.
     */
    bool operator<(QueryInterpretation const& rhs) const;

    void clear() {
        m_logtype.clear();
        m_logtype_string = "";
    }

    void append_logtype(QueryInterpretation& suffix);

    void append_static_token(std::string const& query_substring) {
        StaticQueryToken static_query_token(query_substring);
        if (auto& prev_token = m_logtype.back();
            false == m_logtype.empty() && std::holds_alternative<StaticQueryToken>(prev_token))
        {
            std::get<StaticQueryToken>(prev_token).append(static_query_token);
        } else {
            m_logtype.emplace_back(static_query_token);
        }
    }

    void append_variable_token(
            uint32_t const variable_type,
            std::string query_substring,
            bool const contains_wildcard,
            bool const is_encoded
    ) {
        m_logtype.emplace_back(VariableQueryToken(
                variable_type,
                std::move(query_substring),
                contains_wildcard,
                is_encoded
        ));
    }

    /**
     * Generates the logtype string to compare against the logtype dictionary in the archive.
     * @param lexer
     */
    void generate_logtype_string(log_surgeon::lexers::ByteLexer& lexer);

    [[nodiscard]] uint32_t get_logtype_size() const { return m_logtype.size(); }

    [[nodiscard]] std::variant<StaticQueryToken, VariableQueryToken> const& get_logtype_token(
            uint32_t const i
    ) const {
        return m_logtype[i];
    }

    [[nodiscard]] std::string const& get_logtype_string() const { return m_logtype_string; }

    static constexpr char cIntVarName[] = "int";
    static constexpr char cFloatVarName[] = "float";

private:
    std::vector<std::variant<StaticQueryToken, VariableQueryToken>> m_logtype;
    std::string m_logtype_string;
};

/**
 * Convert input query logtype to string for output
 * @param os
 * @param query_logtype
 * @return output stream with the query logtype
 */
std::ostream& operator<<(std::ostream& os, QueryInterpretation const& query_logtype);
}  // namespace clp

#endif  // CLP_GREP_QUERY_INTERPRETATION_HPP
