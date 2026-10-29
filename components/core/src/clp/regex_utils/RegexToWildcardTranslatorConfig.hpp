#ifndef CLP_REGEX_UTILS_REGEXTOWILDCARDTRANSLATORCONFIG_HPP
#define CLP_REGEX_UTILS_REGEXTOWILDCARDTRANSLATORCONFIG_HPP

namespace clp::regex_utils {
/**
 * Allows users to customize and fine tune how to translate a regex string to wildcard.
 *
 * This class won't affect the core logic and state trasition mechanics of the regex to wildcard
 * translator, but it can make the translator more versatile. For detailed descriptions of how each
 * option should be used, see the getter function docstrings.
 */
class RegexToWildcardTranslatorConfig {
public:
    RegexToWildcardTranslatorConfig(
            bool case_insensitive_wildcard,
            bool add_prefix_suffix_wildcards
    )
            : m_case_insensitive_wildcard{case_insensitive_wildcard},
              m_add_prefix_suffix_wildcards{add_prefix_suffix_wildcards} {}

    /**
     * @return True if the final translated wildcard string will be fed into a case-insensitive
     * wildcard analyzer. In such cases, we can safely translate charset patterns such as [aA] [Bb]
     * into singular lowercase characters a, b.
     */
    [[nodiscard]] auto case_insensitive_wildcard() const -> bool {
        return m_case_insensitive_wildcard;
    }

    /**
     * @return True if in the absense of starting or ending anchors in the regex string, we append
     * prefix or suffix zero or more characters wildcards. In other words, this config is true if
     * the search is a substring search, and false if the search is an exact search.
     */
    [[nodiscard]] auto add_prefix_suffix_wildcards() const -> bool {
        return m_add_prefix_suffix_wildcards;
    }

private:
    // Variables
    bool m_case_insensitive_wildcard;
    bool m_add_prefix_suffix_wildcards;
};
}  // namespace clp::regex_utils

#endif  // CLP_REGEX_UTILS_REGEXTOWILDCARDTRANSLATORCONFIG_HPP
