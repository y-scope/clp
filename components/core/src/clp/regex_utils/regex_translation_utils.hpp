#ifndef CLP_REGEX_UTILS_REGEX_UTILS_HPP
#define CLP_REGEX_UTILS_REGEX_UTILS_HPP

#include <string>
#include <string_view>

#include <outcome/single-header/outcome.hpp>

#include "regex_utils/RegexToWildcardTranslatorConfig.hpp"

namespace clp::regex_utils {

/**
 * Translate a given regex string to wildcard with the default configuration that has all the
 * options set to false.
 *
 * @param regex_str The regex string to be translated.
 * @return The translated wildcard string.
 */
[[nodiscard]] auto regex_to_wildcard(std::string_view regex_str)
        -> OUTCOME_V2_NAMESPACE::std_result<std::string>;

/**
 * Translate a given regex string to wildcard with a custom configuration.
 *
 * @param regex_str The regex string to be translated.
 * @return The translated wildcard string.
 */
[[nodiscard]] auto
regex_to_wildcard(std::string_view regex_str, RegexToWildcardTranslatorConfig const& config)
        -> OUTCOME_V2_NAMESPACE::std_result<std::string>;

}  // namespace clp::regex_utils

#endif  // CLP_REGEX_UTILS_REGEX_UTILS_HPP
