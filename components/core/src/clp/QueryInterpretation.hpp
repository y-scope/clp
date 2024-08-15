#ifndef CLP_GREP_QUERY_INTERPRETATION_HPP
#define CLP_GREP_QUERY_INTERPRETATION_HPP

#include <string>
#include <variant>
#include <vector>

namespace clp {
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

    QueryInterpretation(
            std::variant<char, int> const& val,
            std::string const& string,
            bool var_contains_wildcard
    ) {
        append_value(val, string, var_contains_wildcard);
    }

    bool operator==(QueryInterpretation const& rhs) const = default;

    /**
     * @param rhs
     * @return true if the current logtype is shorter than rhs, false if the current logtype
     * is longer. If equally long, true if the current logtype is lexicographically smaller than
     * rhs, false if bigger. If the logtypes are identical, true if the current search query is
     * lexicographically smaller than rhs, false if bigger. If the search queries are identical,
     * true if the first mismatch in special character locations is a non-special character for the
     * current logtype, false otherwise.
     */
    bool operator<(QueryInterpretation const& rhs) const;

    /**
     * Append a logtype to the current logtype.
     * @param suffix
     */
    void append_logtype(QueryInterpretation& suffix);

    /**
     * Append a single value to the current logtype.
     * @param val
     * @param string
     * @param var_contains_wildcard
     * @param is_encoded_with_wildcard
     */
    void append_value(
            std::variant<char, int> const& val,
            std::string const& string,
            bool var_contains_wildcard,
            bool is_encoded_with_wildcard = false
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

    [[nodiscard]] bool get_var_has_wildcard(uint32_t i) const { return m_var_has_wildcard[i]; }

private:
    std::vector<std::variant<char, int>> m_logtype;
    std::vector<std::string> m_query;
    std::vector<bool> m_is_encoded_with_wildcard;
    std::vector<bool> m_var_has_wildcard;
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
