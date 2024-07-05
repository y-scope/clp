#ifndef CLP_REGEX_UTILS_REGEX_UTILS_HPP
#define CLP_REGEX_UTILS_REGEX_UTILS_HPP

#include <string>

namespace clp::regex_utils {

std::string regexToWildcard(std::string const& regexStr);

/**
 * If a regex expression contains multiple starting or ending anchors, remove the duplicates.
 *
 * @param regexStr
 * @return Trimmed the regex string, leaving at most one starting or ending anchor.
 */
std::string regexTrimLineAnchors(std::string const& regexStr);

/**
 * Check if a regex string has a starting anchor character `^` (caret).
 *
 * @param regexStr
 * @return True if the regex string begins with `^`, false otherwise.
 */
bool regexHasStartAnchor(std::string const& regexStr);


/**
 * Check if a regex string has an ending anchor character `$` (dollar sign).
 * Note that the regex string may end with an escaped `$`, in which case the `$` character retain
 * its literal meaning.
 *
 * @param regexStr
 * @return True if the regex string ends with an unescaped `$`, false otherwise.
 */
bool regexHasEndAnchor(std::string const& regexStr);
}  // namespace clp::regex_utils

#endif  // CLP_REGEX_UTILS_REGEX_UTILS_HPP
