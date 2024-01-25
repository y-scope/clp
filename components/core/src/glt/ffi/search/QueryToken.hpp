#ifndef GLT_FFI_SEARCH_QUERYTOKEN_HPP
#define GLT_FFI_SEARCH_QUERYTOKEN_HPP

#include <string_view>

namespace glt::ffi::search {
enum class TokenType {
    StaticText = 0,
    IntegerVariable,
    FloatVariable,
    DictionaryVariable
};

/**
 * Class representing a token in a query. Note that the original query string is stored by
 * reference, so it must remain valid while the token exists.
 */
class QueryToken {
public:
    // Constructors
    QueryToken(std::string_view query, size_t begin_pos, size_t end_pos)
            : m_query(query),
              m_begin_pos(begin_pos),
              m_end_pos(end_pos),
              m_type(TokenType::StaticText) {}

    // Methods
    bool operator==(QueryToken const& rhs) const {
        return m_query == rhs.m_query && m_begin_pos == rhs.m_begin_pos
               && m_end_pos == rhs.m_end_pos && m_type == rhs.m_type;
    }

    bool operator!=(QueryToken const& rhs) const { return !(rhs == *this); }

    [[nodiscard]] size_t get_begin_pos() const { return m_begin_pos; }

    [[nodiscard]] size_t get_end_pos() const { return m_end_pos; }

    [[nodiscard]] std::string_view get_value() const {
        return m_query.substr(m_begin_pos, m_end_pos - m_begin_pos);
    }

protected:
    std::string_view m_query;
    size_t m_begin_pos;
    size_t m_end_pos;
    TokenType m_type;
};
}  // namespace glt::ffi::search

#endif  // GLT_FFI_SEARCH_QUERYTOKEN_HPP
