#include "regex_utils/regex_translation_utils.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#include <outcome/single-header/outcome.hpp>
#include <string_utils/string_utils.hpp>

#include "regex_utils/constants.hpp"
#include "regex_utils/ErrorCode.hpp"
#include "regex_utils/RegexToWildcardTranslatorConfig.hpp"

namespace clp::regex_utils {
using clp::string_utils::is_alphabet;
using std::error_code;
using std::optional;
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
     *   <li>Normal: The initial state, where characters have no special meanings and are treated
     * literally.</li>
     *   <li>Dot: Encountered a period `.`. Expecting wildcard expression.</li>
     *   <li>Escaped: Encountered a backslash `\`. Expecting an escape sequence.</li>
     *   <li>Charset: Encountered an opening square bracket `[`. Expecting a character set.</li>
     *   <li>CharsetEscaped: Encountered an escape backslash in the character set.</li>
     *   <li>End: Encountered a dollar sign `$`, meaning the regex string has reached the end
     * anchor.</li>
     * </ul>
     */
    enum class RegexPatternState : uint8_t {
        Normal = 0,
        Dot,
        Escaped,
        Charset,
        CharsetEscaped,
        End,
    };

    // Constructor
    TranslatorState() = default;

    // Getters
    [[nodiscard]] auto get_state() const -> RegexPatternState { return m_state; }

    [[nodiscard]] auto get_charset_begin_it() const -> optional<string_view::const_iterator> {
        return m_charset_begin_it;
    }

    // Setters
    auto set_next_state(RegexPatternState const& state) -> void { m_state = state; }

    auto set_charset_begin_it(string_view::const_iterator charset_begin_it) -> void {
        m_charset_begin_it = charset_begin_it;
    }

    auto invalidate_charset_begin_it() -> void { m_charset_begin_it.reset(); }

private:
    // Members
    RegexPatternState m_state{RegexPatternState::Normal};
    optional<string_view::const_iterator> m_charset_begin_it;
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
 * @param[in] config The translator config predefined by the user.
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
 * Appends an escaped regex metacharacter as a literal character to the wildcard string by
 * discarding its preceding backslash.
 *
 * The preceding backslash must be kept for characters that also have special meanings in the
 * wildcard syntax, e.g. `abc.\*xyz` should be translated into `abc?\*xyz` instead of `abc?*xyz`.
 */
[[nodiscard]] StateTransitionFuncSig escaped_state_transition;

/**
 * Reduces a regex character set into a single character so that the regex string is still
 * translatable into a wildcard string.
 *
 * In most cases, only a trival character set containing a single character is reducable. However,
 * if the output wildcard query will be analyzed in case-insensitive mode, character set patterns
 * such as [aA] [Bb] are also reducable. Does not support empty charsets.
 * On error, returns IncompleteCharsetStructure, UnsupportedCharsetPattern, or IllegalState.
 */
[[nodiscard]] StateTransitionFuncSig charset_state_transition;

/**
 * A transient state used to defer handling of escape sequences in a charset pattern.
 *
 * Allows the charset state to accurately capture the appearance of a closing bracket `]`.
 */
[[nodiscard]] StateTransitionFuncSig charset_escaped_state_transition;

/**
 * Disallows the appearances of other characters after encountering an end anchor in the string.
 */
[[nodiscard]] StateTransitionFuncSig end_state_transition;

/**
 * States other than the Normal state may require special handling after the whole regex string has
 * been scanned and processed.
 */
[[nodiscard]] StateTransitionFuncSig final_state_cleanup;

/**
 * Appends a single character as a literal to the wildcard string.
 *
 * If the literal is a metacharacter in the wildcard syntax, prepend the literal with an escape
 * backslash.
 * @param ch The literal to be appended.
 * @param wildcard_str The wildcard string to be updated.
 */
auto append_char_to_wildcard(char ch, string& wildcard_str) -> void;

/**
 * @param ch0
 * @param ch1
 * @return Whether the given chars are the same, but with opposing letter cases, e.g. 'A' vs. 'a'.
 */
[[nodiscard]] auto is_same_char_opposite_case(char ch0, char ch1) -> bool;

auto normal_state_transition(
        TranslatorState& state,
        string_view::const_iterator& it,
        string& wildcard_str,
        [[maybe_unused]] RegexToWildcardTranslatorConfig const& config
) -> error_code {
    auto const ch{*it};
    switch (ch) {
        case '.':
            state.set_next_state(TranslatorState::RegexPatternState::Dot);
            break;
        case cEscapeChar:
            state.set_next_state(TranslatorState::RegexPatternState::Escaped);
            break;
        case '[':
            state.set_charset_begin_it(it + 1);
            state.set_next_state(TranslatorState::RegexPatternState::Charset);
            break;
        case cRegexEndAnchor:
            state.set_next_state(TranslatorState::RegexPatternState::End);
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
    state.set_next_state(TranslatorState::RegexPatternState::Normal);
    return ErrorCode::Success;
}

auto escaped_state_transition(
        TranslatorState& state,
        string_view::const_iterator& it,
        string& wildcard_str,
        [[maybe_unused]] RegexToWildcardTranslatorConfig const& config
) -> error_code {
    auto const ch{*it};
    if (false == cRegexEscapeSeqMetaCharsLut.at(ch)) {
        return ErrorCode::IllegalEscapeSequence;
    }
    append_char_to_wildcard(ch, wildcard_str);
    state.set_next_state(TranslatorState::RegexPatternState::Normal);
    return ErrorCode::Success;
}

auto charset_state_transition(
        TranslatorState& state,
        string_view::const_iterator& it,
        string& wildcard_str,
        RegexToWildcardTranslatorConfig const& config
) -> error_code {
    auto const charset_begin_it_opt{state.get_charset_begin_it()};
    if (false == charset_begin_it_opt.has_value()) {
        return ErrorCode::IllegalState;
    }
    string_view::const_iterator const charset_begin_it = charset_begin_it_opt.value();

    auto const ch{*it};
    if (cEscapeChar == ch) {
        state.set_next_state(TranslatorState::RegexPatternState::CharsetEscaped);
        return ErrorCode::Success;
    }
    if (']' != ch) {
        return ErrorCode::Success;
    }

    auto const charset_len{it - charset_begin_it};
    if (0 == charset_len || charset_len > 2) {
        return ErrorCode::UnsupportedCharsetPattern;
    }

    auto const ch0{*charset_begin_it};
    auto const ch1{*(charset_begin_it + 1)};
    char parsed_char{};

    if (1 == charset_len) {
        if (cCharsetNegate == ch0 || cEscapeChar == ch0) {
            return ErrorCode::UnsupportedCharsetPattern;
        }
        parsed_char = ch0;
    } else {  // 2 == charset_len
        if (cEscapeChar == ch0 && cRegexCharsetEscapeSeqMetaCharsLut.at(ch1)) {
            parsed_char = ch1;
        } else if (config.case_insensitive_wildcard() && is_same_char_opposite_case(ch0, ch1)) {
            parsed_char = ch0 > ch1 ? ch0 : ch1;  // choose the lower case character
        } else {
            return ErrorCode::UnsupportedCharsetPattern;
        }
    }

    append_char_to_wildcard(parsed_char, wildcard_str);
    state.invalidate_charset_begin_it();
    state.set_next_state(TranslatorState::RegexPatternState::Normal);
    return ErrorCode::Success;
}

auto charset_escaped_state_transition(
        TranslatorState& state,
        [[maybe_unused]] string_view::const_iterator& it,
        [[maybe_unused]] string& wildcard_str,
        [[maybe_unused]] RegexToWildcardTranslatorConfig const& config
) -> error_code {
    state.set_next_state(TranslatorState::RegexPatternState::Charset);
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
        case TranslatorState::RegexPatternState::Dot:
            // The last character is a single `.`, without the possibility of becoming a
            // multichar wildcard
            wildcard_str += cSingleCharWildcard;
            break;
        case TranslatorState::RegexPatternState::Charset:
        case TranslatorState::RegexPatternState::CharsetEscaped:
            return ErrorCode::IncompleteCharsetStructure;
        default:
            break;
    }

    if (TranslatorState::RegexPatternState::End != state.get_state()
        && config.add_prefix_suffix_wildcards())
    {
        wildcard_str += cZeroOrMoreCharsWildcard;
    }
    return ErrorCode::Success;
}

auto append_char_to_wildcard(char ch, string& wildcard_str) -> void {
    if (cWildcardMetaCharsLut.at(ch)) {
        wildcard_str += cEscapeChar;
    }
    wildcard_str += ch;
}

auto is_same_char_opposite_case(char ch0, char ch1) -> bool {
    int const upper_lower_case_ascii_offset{'a' - 'A'};
    return (is_alphabet(ch0) && is_alphabet(ch1)
            && (((ch0 - ch1) == upper_lower_case_ascii_offset)
                || ((ch1 - ch0) == upper_lower_case_ascii_offset)));
}
}  // namespace

auto regex_to_wildcard(string_view regex_str) -> OUTCOME_V2_NAMESPACE::std_result<string> {
    return regex_to_wildcard(
            regex_str,
            {/*case_insensitive_wildcard=*/false, /*add_prefix_suffix_wildcards=*/false}
    );
}

auto regex_to_wildcard(string_view regex_str, RegexToWildcardTranslatorConfig const& config)
        -> OUTCOME_V2_NAMESPACE::std_result<string> {
    if (regex_str.empty()) {
        return string{};
    }

    string_view::const_iterator it{regex_str.cbegin()};
    string wildcard_str;
    TranslatorState state;

    // If there is no starting anchor character, append multichar wildcard prefix
    if (cRegexStartAnchor == *it) {
        ++it;
    } else if (config.add_prefix_suffix_wildcards()) {
        wildcard_str += cZeroOrMoreCharsWildcard;
    }

    error_code ec{};
    while (it != regex_str.cend()) {
        switch (state.get_state()) {
            case TranslatorState::RegexPatternState::Normal:
                ec = normal_state_transition(state, it, wildcard_str, config);
                break;
            case TranslatorState::RegexPatternState::Dot:
                ec = dot_state_transition(state, it, wildcard_str, config);
                break;
            case TranslatorState::RegexPatternState::Escaped:
                ec = escaped_state_transition(state, it, wildcard_str, config);
                break;
            case TranslatorState::RegexPatternState::Charset:
                ec = charset_state_transition(state, it, wildcard_str, config);
                break;
            case TranslatorState::RegexPatternState::CharsetEscaped:
                ec = charset_escaped_state_transition(state, it, wildcard_str, config);
                break;
            case TranslatorState::RegexPatternState::End:
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
