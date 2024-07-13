#ifndef CLP_REGEX_UTILS_REGEX_UTILS_HPP
#define CLP_REGEX_UTILS_REGEX_UTILS_HPP

#include <string>
#include <string_view>

#include <boost-outcome/include/boost/outcome/config.hpp>
#include <boost-outcome/include/boost/outcome/std_result.hpp>

#include "regex_utils/RegexToWildcardTranslatorConfig.hpp"

namespace clp::regex_utils {

[[nodiscard]] auto regex_to_wildcard(std::string_view regex_str
) -> BOOST_OUTCOME_V2_NAMESPACE::std_result<std::string>;

[[nodiscard]] auto regex_to_wildcard(
        std::string_view regex_str,
        RegexToWildcardTranslatorConfig const& config
) -> BOOST_OUTCOME_V2_NAMESPACE::std_result<std::string>;

/**
 * If a regex expression contains multiple starting or ending anchors, remove the duplicates.
 *
 * @param regex_str
 * @return Trimmed the regex string, leaving at most one starting or ending anchor.
 */
[[nodiscard]] auto regex_trim_line_anchors(std::string_view regex_str) -> std::string;

/**
 * Check if a regex string has a starting anchor character `^` (caret).
 *
 * @param regex_str
 * @return True if the regex string begins with `^`, false otherwise.
 */
[[nodiscard]] auto regex_has_start_anchor(std::string_view regex_str) -> bool;

/**
 * Check if a regex string has an ending anchor character `$` (dollar sign).
 * Note that the regex string may end with an escaped `$`, in which case the `$` character retain
 * its literal meaning.
 *
 * @param regex_str
 * @return True if the regex string ends with an unescaped `$`, false otherwise.
 */
[[nodiscard]] auto regex_has_end_anchor(std::string_view regex_str) -> bool;
}  // namespace clp::regex_utils

#endif  // CLP_REGEX_UTILS_REGEX_UTILS_HPP
