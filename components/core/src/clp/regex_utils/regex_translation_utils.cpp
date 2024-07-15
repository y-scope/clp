#include "regex_utils/regex_translation_utils.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>

#include <boost-outcome/include/boost/outcome/config.hpp>
#include <boost-outcome/include/boost/outcome/std_result.hpp>

#include "regex_utils/constants.hpp"
#include "regex_utils/ErrorCode.hpp"
#include "regex_utils/RegexToWildcardTranslatorConfig.hpp"

using namespace clp::regex_utils;

using std::error_code;
using std::string;
using std::string_view;

namespace {
// Internal utility class and function declarations
/**
 * Class for storing regex translation analysis states, capture group, quantifier information, etc.
 */
class TranslatorState;

// State transition functions common signature
using StateTransitionFunc
        = auto(TranslatorState&,
               string_view::const_iterator&,
               string&,
               RegexToWildcardTranslatorConfig const&) -> error_code;

// State transition functions whose names correspond to the current analysis state.
[[nodiscard]] StateTransitionFunc normal_state_transition;
[[nodiscard]] StateTransitionFunc dot_state_transition;
[[nodiscard]] StateTransitionFunc end_state_transition;
[[nodiscard]] StateTransitionFunc final_state_cleanup;

class TranslatorState {
public:
    // Regex translation pattern analysis states.
    enum class RegexPatternState : uint8_t {
        // The initial state, where characters have no special meanings and are treated literally.
        NORMAL = 0,
        // Encountered a period `.`. Expecting wildcard expression.
        DOT,
        // Encountered a dollar sign `$`, meaning the regex string has reached the end anchor.
        END,
    };

    // Constructor
    TranslatorState() = default;

    // Getters
    [[nodiscard]] auto get_state() const -> RegexPatternState { return m_state; }

    // Setters
    auto set_next_state(RegexPatternState const& state) -> void { m_state = state; }

private:
    // Variables
    RegexPatternState m_state{RegexPatternState::NORMAL};
};
}  // namespace

namespace clp::regex_utils {
// Main API
auto regex_to_wildcard(string_view regex_str) -> BOOST_OUTCOME_V2_NAMESPACE::std_result<string> {
    RegexToWildcardTranslatorConfig const default_config{false, false};
    return regex_to_wildcard(regex_str, default_config);
}

auto regex_to_wildcard(string_view regex_str, RegexToWildcardTranslatorConfig const& config)
        -> BOOST_OUTCOME_V2_NAMESPACE::std_result<string> {
    if (regex_str.empty()) {
        return string();
    }

    // Initialize translation state, scan position, and return string
    TranslatorState state;
    string_view::const_iterator it = regex_str.cbegin();
    string wildcard_str;

    // If there is no starting anchor character, append multichar wildcard prefix
    if (cRegexStartAnchor == *it) {
        ++it;
    } else if (config.add_prefix_suffix_wildcards()) {
        wildcard_str += cZeroOrMoreCharsWildcard;
    }

    error_code ec{};
    while (it != regex_str.end()) {
        // Main state transition table
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

    // Do the final state check and clean up
    ec = final_state_cleanup(state, it, wildcard_str, config);
    if (ec) {
        return ec;
    }
    return wildcard_str;
}
}  // namespace clp::regex_utils

namespace {

auto normal_state_transition(
        TranslatorState& state,
        string_view::const_iterator& it,
        string& wildcard_str,
        RegexToWildcardTranslatorConfig const& /*config*/
) -> error_code {
    char const ch = *it;
    switch (ch) {
        case '.':
            state.set_next_state(TranslatorState::RegexPatternState::DOT);
            break;
        case cRegexEndAnchor:
            state.set_next_state(TranslatorState::RegexPatternState::END);
            break;
        case cRegexZeroOrMore:
            return ErrorCode::Star;
        case cRegexOneOrMore:
            return ErrorCode::Plus;
        case cRegexZeroOrOne:
            return ErrorCode::Question;
        case '|':
            return ErrorCode::Pipe;
        case cRegexStartAnchor:
            return ErrorCode::Caret;
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
        RegexToWildcardTranslatorConfig const& /*config*/
) -> error_code {
    switch (*it) {
        case cZeroOrMoreCharsWildcard:
            // .* gets translated to *
            wildcard_str += cZeroOrMoreCharsWildcard;
            break;
        case cRegexOneOrMore:
            // .+ gets translated to ?*
            wildcard_str = wildcard_str + cSingleCharWildcard + cZeroOrMoreCharsWildcard;
            break;
        default:
            // . gets translated to ?
            wildcard_str += cSingleCharWildcard;
            // Backtrack the scan by one position to handle the current char in the next iteration.
            --it;
            break;
    }
    state.set_next_state(TranslatorState::RegexPatternState::NORMAL);
    return ErrorCode::Success;
}

auto end_state_transition(
        TranslatorState& /*state*/,
        string_view::const_iterator& it,
        string& /*wildcard_str*/,
        RegexToWildcardTranslatorConfig const& /*config*/
) -> error_code {
    if (cRegexEndAnchor != *it) {
        return ErrorCode::Dollar;
    }
    return ErrorCode::Success;
}

auto final_state_cleanup(
        TranslatorState& state,
        string_view::const_iterator& /*it*/,
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
