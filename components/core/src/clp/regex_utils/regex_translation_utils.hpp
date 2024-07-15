#ifndef CLP_REGEX_UTILS_REGEX_UTILS_HPP
#define CLP_REGEX_UTILS_REGEX_UTILS_HPP

#include <string>
#include <string_view>

#include <boost-outcome/include/boost/outcome/config.hpp>
#include <boost-outcome/include/boost/outcome/std_result.hpp>

#include "regex_utils/RegexToWildcardTranslatorConfig.hpp"

namespace clp::regex_utils {

/**
 * Call the regex to wildcard translation function with a default configuration that has all the
 * options as false. For more details on the config options, see
 * RegexToWildcardTranslatorConfig.hpp.
 *
 * @param regex_str The regex string to be translated.
 * @return The translated wildcard string.
 */
[[nodiscard]] auto regex_to_wildcard(std::string_view regex_str
) -> BOOST_OUTCOME_V2_NAMESPACE::std_result<std::string>;

/**
 * Translated a given regex string to wildcard with a custom configuration. For more details on the
 * config options, see RegexToWildcardTranslatorConfig.hpp.
 *
 * @param regex_str The regex string to be translated.
 * @return The translated wildcard string.
 */
[[nodiscard]] auto regex_to_wildcard(
        std::string_view regex_str,
        RegexToWildcardTranslatorConfig const& config
) -> BOOST_OUTCOME_V2_NAMESPACE::std_result<std::string>;

}  // namespace clp::regex_utils

#endif  // CLP_REGEX_UTILS_REGEX_UTILS_HPP
