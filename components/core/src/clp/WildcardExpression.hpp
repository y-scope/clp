#ifndef CLP_WILDCARDEXPRESSION_HPP
#define CLP_WILDCARDEXPRESSION_HPP

#include <cstdint>
#include <string>
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
        return m_search_string_ptr->substr(m_begin_idx, m_end_idx - m_begin_idx);
    }

private:
    WildcardExpression const* m_search_string_ptr;
    uint32_t m_begin_idx;
    uint32_t m_end_idx;
};
}  // namespace clp

#endif  // CLP_WILDCARDEXPRESSION_HPP
