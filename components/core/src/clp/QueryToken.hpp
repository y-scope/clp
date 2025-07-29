#ifndef CLP_QUERYTOKEN_HPP
#define CLP_QUERYTOKEN_HPP

#include <cstddef>
#include <string>
#include <vector>

namespace clp {
// Class representing a token in a query. It is used to interpret a token in a search string.
class QueryToken {
public:
    // Constructors
    QueryToken(std::string const& query_string, size_t begin_pos, size_t end_pos, bool is_var);

    // Methods
    bool cannot_convert_to_non_dict_var() const;
    bool contains_wildcards() const;
    bool has_greedy_wildcard_in_middle() const;
    bool has_prefix_greedy_wildcard() const;
    bool has_suffix_greedy_wildcard() const;
    bool is_ambiguous_token() const;
    bool is_float_var() const;
    bool is_int_var() const;
    bool is_var() const;
    bool is_wildcard() const;

    size_t get_begin_pos() const;
    size_t get_end_pos() const;
    std::string const& get_value() const;

    bool change_to_next_possible_type();

private:
    // Types
    // Type for the purpose of generating different subqueries. E.g., if a token is of type
    // DictOrIntVar, it would generate a different subquery than if it was of type Logtype.
    enum class Type {
        Wildcard,
        // Ambiguous indicates the token can be more than one of the types listed below
        Ambiguous,
        Logtype,
        DictionaryVar,
        FloatVar,
        IntVar
    };

    // Variables
    bool m_cannot_convert_to_non_dict_var{};
    bool m_contains_wildcards{};
    bool m_has_greedy_wildcard_in_middle{};
    bool m_has_prefix_greedy_wildcard{};
    bool m_has_suffix_greedy_wildcard{};

    size_t m_begin_pos{};
    size_t m_end_pos{};
    std::string m_value;

    // Type if variable has unambiguous type
    Type m_type{};
    // Types if variable type is ambiguous
    std::vector<Type> m_possible_types;
    // Index of the current possible type selected for generating a subquery
    size_t m_current_possible_type_ix{};
};
}  // namespace clp
#endif  // CLP_QUERYTOKEN_HPP
