#ifndef CLP_REGEX_UTILS_REGEXTOWILDCARDTRANSLATORCONFIG_HPP
#define CLP_REGEX_UTILS_REGEXTOWILDCARDTRANSLATORCONFIG_HPP

namespace clp::regex_utils {

class RegexToWildcardTranslatorConfig {
public:
    // Constructors
    RegexToWildcardTranslatorConfig() = default;

    // Getters
    [[nodiscard]] auto case_insensitive_wildcard() const -> bool {
        return m_case_insensitive_wildcard;
    }

    [[nodiscard]] auto allow_anchors() const -> bool { return m_allow_anchors; }

    [[nodiscard]] auto add_prefix_suffix_wildcards() const -> bool {
        return m_add_prefix_suffix_wildcards;
    }

    // Setters
    void set_case_insensitive_wildcard(bool case_insensitive_wildcard) {
        m_case_insensitive_wildcard = case_insensitive_wildcard;
    }

    void set_allow_anchors(bool allow_anchors) { m_allow_anchors = allow_anchors; }

    void set_add_prefix_suffix_wildcards(bool add_prefix_suffix_wildcards) {
        m_add_prefix_suffix_wildcards = add_prefix_suffix_wildcards;
    }

private:
    // Variables
    bool m_case_insensitive_wildcard = false;
    bool m_allow_anchors = true;
    bool m_add_prefix_suffix_wildcards = false;
};

}  // namespace clp::regex_utils

#endif  // CLP_REGEX_UTILS_REGEXTOWILDCARDTRANSLATORCONFIG_HPP
