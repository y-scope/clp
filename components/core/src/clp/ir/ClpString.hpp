#ifndef CLP_IR_CLPSTRING_HPP
#define CLP_IR_CLPSTRING_HPP

#include <string>
#include <utility>
#include <vector>

#include "types.hpp"

namespace clp::ir {
/**
 * A class representing a CLP string encoded using CLP's IR.
 * @tparam encoded_variable_t The type of encoded variables in the event.
 */
template <typename encoded_variable_t>
class ClpString {
public:
    // Constructor
    explicit ClpString(
            std::string logtype,
            std::vector<std::string> dict_vars,
            std::vector<encoded_variable_t> encoded_vars
    )
            : m_logtype{std::move(logtype)},
              m_dict_vars{std::move(dict_vars)},
              m_encoded_vars{std::move(encoded_vars)} {}

    // Disable copy constructor and assignment operator
    ClpString(ClpString const&) = delete;
    auto operator=(ClpString const&) -> ClpString& = delete;

    // Default move constructor and assignment operator
    ClpString(ClpString&&) = default;
    auto operator=(ClpString&&) -> ClpString& = default;

    // Destructor
    ~ClpString() = default;

    // Methods
    [[nodiscard]] auto get_logtype() const -> std::string const& { return m_logtype; }

    [[nodiscard]] auto get_dict_vars() const -> std::vector<std::string> const& {
        return m_dict_vars;
    }

    [[nodiscard]] auto get_encoded_vars() const -> std::vector<encoded_variable_t> const& {
        return m_encoded_vars;
    }

private:
    // Variables
    std::string m_logtype;
    std::vector<std::string> m_dict_vars;
    std::vector<encoded_variable_t> m_encoded_vars;
};

using EightByteEncodingClpString = ClpString<eight_byte_encoded_variable_t>;
using FourByteEncodingClpString = ClpString<four_byte_encoded_variable_t>;
}  // namespace clp::ir

#endif  // CLP_IR_CLPSTRING_HPP
