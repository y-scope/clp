#ifndef CLP_GREP_QUERY_INTERPRETATION_HPP
#define CLP_GREP_QUERY_INTERPRETATION_HPP

#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <log_surgeon/Lexer.hpp>

namespace clp {
class WildcardExpressionView;

/**
 * A pattern that supports two types of wildcards:
 * - `*` matches zero or more characters
 * - '?' matches any single character
 *
 * To search for a literal `*` or `?`, the pattern should escape it with a backslash (`\`).
 */
class WildcardExpression {
public:
    explicit WildcardExpression(std::string processed_search_string);

    [[nodiscard]] auto
    substr(uint32_t const begin_idx, uint32_t const length) const -> std::string {
        return m_processed_search_string.substr(begin_idx, length);
    }

    [[nodiscard]] auto
    create_view(uint32_t start_idx, uint32_t end_idx) const -> WildcardExpressionView;

    [[nodiscard]] auto length() const -> uint32_t { return m_processed_search_string.size(); }

    [[nodiscard]] auto get_value_is_greedy_wildcard(uint32_t const idx) const -> bool {
        return m_is_greedy_wildcard[idx];
    }

    [[nodiscard]] auto get_value_is_non_greedy_wildcard(uint32_t const idx) const -> bool {
        return m_is_non_greedy_wildcard[idx];
    }

    [[nodiscard]] auto get_value_is_escape(uint32_t const idx) const -> bool {
        return m_is_escape[idx];
    }

    [[nodiscard]] auto get_value(uint32_t const idx) const -> char {
        return m_processed_search_string[idx];
    }

    [[nodiscard]] auto
    get_substr_copy(uint32_t const begin_idx, uint32_t const length) const -> std::string {
        return m_processed_search_string.substr(begin_idx, length);
    }

private:
    std::vector<bool> m_is_greedy_wildcard;
    std::vector<bool> m_is_non_greedy_wildcard;
    std::vector<bool> m_is_escape;
    std::string m_processed_search_string;
};

/**
 * A view of a WildcardExpression.
 */
class WildcardExpressionView {
public:
    WildcardExpressionView(
            WildcardExpression const* search_string_ptr,
            uint32_t const begin_idx,
            uint32_t const end_idx

    )
            : m_search_string_ptr(search_string_ptr),
              m_begin_idx(begin_idx),
              m_end_idx(end_idx) {}

    /**
     * @return A copy of this view, but extended to include adjacent greedy wildcards.
     */
    [[nodiscard]] auto extend_to_adjacent_greedy_wildcards() const -> WildcardExpressionView;

    [[nodiscard]] auto is_greedy_wildcard() const -> bool {
        return 1 == length() && m_search_string_ptr->get_value_is_greedy_wildcard(m_begin_idx);
    }

    [[nodiscard]] auto is_non_greedy_wildcard() const -> bool {
        return 1 == length() && m_search_string_ptr->get_value_is_non_greedy_wildcard(m_begin_idx);
    }

    [[nodiscard]] auto starts_or_ends_with_greedy_wildcard() const -> bool {
        return m_search_string_ptr->get_value_is_greedy_wildcard(m_begin_idx)
               || m_search_string_ptr->get_value_is_greedy_wildcard(m_end_idx - 1);
    }

    /**
     * @param lexer
     * @return Whether the substring in view is surrounded by delimiters or unescaped wildcards.
     * NOTE: This method assumes that the beginning of the viewed string is preceeded by a delimiter
     * and the end is succeeded by a delimiter.
     */
    [[nodiscard]] auto surrounded_by_delims_or_wildcards(log_surgeon::lexers::ByteLexer const& lexer
    ) const -> bool;

    [[nodiscard]] auto length() const -> uint32_t { return m_end_idx - m_begin_idx; }

    [[nodiscard]] auto get_value_is_greedy_wildcard(uint32_t const idx) const -> bool {
        return m_search_string_ptr->get_value_is_greedy_wildcard(m_begin_idx + idx);
    }

    [[nodiscard]] auto get_value_is_non_greedy_wildcard(uint32_t const idx) const -> bool {
        return m_search_string_ptr->get_value_is_non_greedy_wildcard(m_begin_idx + idx);
    }

    [[nodiscard]] auto get_value_is_escape(uint32_t const idx) const -> bool {
        return m_search_string_ptr->get_value_is_escape(m_begin_idx + idx);
    }

    [[nodiscard]] auto get_value(uint32_t const idx) const -> char {
        return m_search_string_ptr->get_value(m_begin_idx + idx);
    }

    [[nodiscard]] auto get_substr_copy() const -> std::string {
        return m_search_string_ptr->get_substr_copy(m_begin_idx, m_end_idx - m_begin_idx);
    }

private:
    WildcardExpression const* m_search_string_ptr;
    uint32_t m_begin_idx;
    uint32_t m_end_idx;
};

/**
 * Represents a static substring in the query string as a token.
 */
class StaticQueryToken {
public:
    explicit StaticQueryToken(std::string query_substring)
            : m_query_substring(std::move(query_substring)) {}

    auto operator==(StaticQueryToken const& rhs) const -> bool = default;

    auto operator!=(StaticQueryToken const& rhs) const -> bool = default;

    auto operator<(StaticQueryToken const& rhs) const -> bool {
        return m_query_substring < rhs.m_query_substring;
    }

    auto operator>(StaticQueryToken const& rhs) const -> bool {
        return m_query_substring > rhs.m_query_substring;
    }

    auto append(StaticQueryToken const& rhs) -> void {
        m_query_substring += rhs.get_query_substring();
    }

    [[nodiscard]] auto get_query_substring() const -> std::string const& {
        return m_query_substring;
    }

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

    auto operator==(VariableQueryToken const& rhs) const -> bool = default;

    auto operator!=(VariableQueryToken const& rhs) const -> bool = default;

    auto operator<(VariableQueryToken const& rhs) const -> bool;

    auto operator>(VariableQueryToken const& rhs) const -> bool;

    [[nodiscard]] auto get_variable_type() const -> uint32_t { return m_variable_type; }

    [[nodiscard]] auto get_query_substring() const -> std::string const& {
        return m_query_substring;
    }

    [[nodiscard]] auto get_has_wildcard() const -> bool { return m_has_wildcard; }

    [[nodiscard]] auto get_is_encoded_with_wildcard() const -> bool {
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
    auto operator==(QueryInterpretation const& rhs) const -> bool {
        return m_logtype == rhs.m_logtype;
    }

    /**
     * @param rhs
     * @return true if the current logtype is shorter than rhs, false if the current logtype
     * is longer. If equally long, true if the current logtype is lexicographically smaller than
     * rhs, false if bigger. If the logtypes are identical, true if the current search query is
     * lexicographically smaller than rhs, false if bigger. If the search queries are identical,
     * true if the first mismatch in special character locations is a non-special character for the
     * current logtype, false otherwise. Ignores m_logtype_string.
     */
    auto operator<(QueryInterpretation const& rhs) const -> bool;

    auto clear() -> void {
        m_logtype.clear();
        m_logtype_string = "";
    }

    auto append_logtype(QueryInterpretation& suffix) -> void;

    auto append_static_token(std::string const& query_substring) -> void {
        StaticQueryToken static_query_token(query_substring);
        if (auto& prev_token = m_logtype.back();
            false == m_logtype.empty() && std::holds_alternative<StaticQueryToken>(prev_token))
        {
            std::get<StaticQueryToken>(prev_token).append(static_query_token);
        } else {
            m_logtype.emplace_back(static_query_token);
        }
    }

    auto append_variable_token(
            uint32_t const variable_type,
            std::string query_substring,
            bool const contains_wildcard,
            bool const is_encoded
    ) -> void {
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
    auto generate_logtype_string(log_surgeon::lexers::ByteLexer& lexer) -> void;

    [[nodiscard]] auto get_logtype_size() const -> uint32_t { return m_logtype.size(); }

    [[nodiscard]] auto get_logtype_token(uint32_t const i
    ) const -> std::variant<StaticQueryToken, VariableQueryToken> const& {
        return m_logtype[i];
    }

    [[nodiscard]] auto get_logtype_string() const -> std::string const& { return m_logtype_string; }

    static constexpr std::string_view cIntVarName = "int";
    static constexpr std::string_view cFloatVarName = "float";

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
auto operator<<(std::ostream& os, QueryInterpretation const& query_logtype) -> std::ostream&;
}  // namespace clp

#endif  // CLP_GREP_QUERY_INTERPRETATION_HPP
