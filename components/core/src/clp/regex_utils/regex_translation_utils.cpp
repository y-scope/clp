#include "regex_utils/regex_translation_utils.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>

#include <outcome/single-header/outcome.hpp>

#include "regex_utils/constants.hpp"
#include "regex_utils/ErrorCode.hpp"
#include "regex_utils/RegexToWildcardTranslatorConfig.hpp"

namespace clp::regex_utils {
using std::error_code;
using std::string;
using std::string_view;

namespace {
/**
 * Class for storing regex translation analysis states, capture group, quantifier information, etc.
 */
class TranslatorState {
public:
    /**
     * States for which we apply specific rules to translate encountered regex patterns.
     *
     * This list may be expanded as the translator supports translating more regex patterns.
     * <ul>
     *   <li>NORMAL: The initial state, where characters have no special meanings and are treated
     * literally.</li>
     *   <li>DOT: Encountered a period `.`. Expecting wildcard expression.</li>
     *   <li>END: Encountered a dollar sign `$`, meaning the regex string has reached the end
     * anchor.</li>
     * </ul>
     */
    enum class RegexPatternState : uint8_t {
        NORMAL = 0,
        DOT,
        END,
    };

    // Constructor
    TranslatorState() = default;

    // Getters
    [[nodiscard]] auto get_state() const -> RegexPatternState { return m_state; }

    // Setters
    auto set_next_state(RegexPatternState const& state) -> void { m_state = state; }

private:
    // Members
    RegexPatternState m_state{RegexPatternState::NORMAL};
};

/**
 * Functions that handle current-state-specific tasks before transitioning to the next state.
 *
 * @param[in, out] state The object that stores translator's internal information. The primary
 * state member variable is always updated if a transition occures. Even if there's no state
 * transition, other analysis info may be updated.
 * @param[in, out] it The iterator that represents the current regex string scan position. May be
 * updated to advance or backtrack the scan position.
 * @param[out] wildcard_str The translated wildcard string. May or may not be updated.
 * @param[in] config The translator config.
 * @return clp::regex_utils::ErrorCode
 */
using StateTransitionFuncSig
        = auto(TranslatorState& state,
               string_view::const_iterator& it,
               string& wildcard_str,
               RegexToWildcardTranslatorConfig const& config) -> error_code;

/**
 * Treats each character literally and directly append it to the wildcard string, unless it is a
 * meta-character.
 *
 * Each meta-character either triggers a state transition, or makes the regex string untranslatable.
 */
[[nodiscard]] StateTransitionFuncSig normal_state_transition;

/**
 * Attempts to translate regex wildcard patterns that start with `.` character.
 *
 * Performs the following translation if possible:
 * <ul>
 *   <li> `.*` gets translated into `*`</li>
 *   <li> `.+` gets translated into `?*`</li>
 *   <li> `.` gets translated into `?`</li>
 * </ul>
 */
[[nodiscard]] StateTransitionFuncSig dot_state_transition;

/**
 * Disallows the appearances of other characters after encountering an end anchor in the string.
 */
[[nodiscard]] StateTransitionFuncSig end_state_transition;

/**
 * States other than the NORMAL state may require special handling after the whole regex string has
 * been scanned and processed.
 */
[[nodiscard]] StateTransitionFuncSig final_state_cleanup;

auto normal_state_transition(
        TranslatorState& state,
        string_view::const_iterator& it,
        string& wildcard_str,
        [[maybe_unused]] RegexToWildcardTranslatorConfig const& config
) -> error_code {
    auto const ch{*it};
    switch (ch) {
        case '.':
            state.set_next_state(TranslatorState::RegexPatternState::DOT);
            break;
        case cRegexEndAnchor:
            state.set_next_state(TranslatorState::RegexPatternState::END);
            break;
        case cRegexZeroOrMore:
            return ErrorCode::UntranslatableStar;
        case cRegexOneOrMore:
            return ErrorCode::UntranslatablePlus;
        case cRegexZeroOrOne:
            return ErrorCode::UnsupportedQuestionMark;
        case '|':
            return ErrorCode::UnsupportedPipe;
        case cRegexStartAnchor:
            return ErrorCode::IllegalCaret;
        case ')':
            return ErrorCode::UnmatchedParenthesis;
        default:
            wildcard_str += ch;
            break;
    }
    return ErrorCode::Success;
}

auto dot_state_transition(
        TranslatorState& state,
        string_view::const_iterator& it,
        string& wildcard_str,
        [[maybe_unused]] RegexToWildcardTranslatorConfig const& config
) -> error_code {
    switch (*it) {
        case cZeroOrMoreCharsWildcard:
            wildcard_str += cZeroOrMoreCharsWildcard;
            break;
        case cRegexOneOrMore:
            wildcard_str = wildcard_str + cSingleCharWildcard + cZeroOrMoreCharsWildcard;
            break;
        default:
            wildcard_str += cSingleCharWildcard;
            // Backtrack the scan by one position to handle the current char in the next iteration.
            --it;
            break;
    }
    state.set_next_state(TranslatorState::RegexPatternState::NORMAL);
    return ErrorCode::Success;
}

auto end_state_transition(
        [[maybe_unused]] TranslatorState& state,
        string_view::const_iterator& it,
        [[maybe_unused]] string& wildcard_str,
        [[maybe_unused]] RegexToWildcardTranslatorConfig const& config
) -> error_code {
    if (cRegexEndAnchor != *it) {
        return ErrorCode::IllegalDollarSign;
    }
    return ErrorCode::Success;
}

auto final_state_cleanup(
        TranslatorState& state,
        [[maybe_unused]] string_view::const_iterator& it,
        string& wildcard_str,
        RegexToWildcardTranslatorConfig const& config
) -> error_code {
    switch (state.get_state()) {
        case TranslatorState::RegexPatternState::DOT:
            // The last character is a single `.`, without the possibility of becoming a
            // multichar wildcard
            wildcard_str += cSingleCharWildcard;
            break;
        default:
            break;
    }

    if (TranslatorState::RegexPatternState::END != state.get_state()
        && config.add_prefix_suffix_wildcards())
    {
        wildcard_str += cZeroOrMoreCharsWildcard;
    }
    return ErrorCode::Success;
}
}  // namespace

auto regex_to_wildcard(string_view regex_str) -> OUTCOME_V2_NAMESPACE::std_result<string> {
    return regex_to_wildcard(regex_str, {false, false});
}

auto regex_to_wildcard(string_view regex_str, RegexToWildcardTranslatorConfig const& config)
        -> OUTCOME_V2_NAMESPACE::std_result<string> {
    if (regex_str.empty()) {
        return string{};
    }

    TranslatorState state;
    string_view::const_iterator it{regex_str.cbegin()};
    string wildcard_str;

    // If there is no starting anchor character, append multichar wildcard prefix
    if (cRegexStartAnchor == *it) {
        ++it;
    } else if (config.add_prefix_suffix_wildcards()) {
        wildcard_str += cZeroOrMoreCharsWildcard;
    }

    error_code ec{};
    while (it != regex_str.cend()) {
        switch (state.get_state()) {
            case TranslatorState::RegexPatternState::NORMAL:
                ec = normal_state_transition(state, it, wildcard_str, config);
                break;
            case TranslatorState::RegexPatternState::DOT:
                ec = dot_state_transition(state, it, wildcard_str, config);
                break;
            case TranslatorState::RegexPatternState::END:
                ec = end_state_transition(state, it, wildcard_str, config);
                break;
            default:
                ec = ErrorCode::IllegalState;
                break;
        }
        if (ec) {
            return ec;
        }
        ++it;
    }

    ec = final_state_cleanup(state, it, wildcard_str, config);
    if (ec) {
        return ec;
    }
    return wildcard_str;
}
}  // namespace clp::regex_utils
