#include "regex_utils/regex_utils.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

#include <boost-outcome/include/boost/outcome/config.hpp>
#include <boost-outcome/include/boost/outcome/std_result.hpp>
#include <string_utils/string_utils.hpp>

#include "regex_utils/constants.hpp"
#include "regex_utils/ErrorCode.hpp"
#include "regex_utils/RegexToWildcardTranslatorConfig.hpp"

using clp::string_utils::is_alphabet;
using clp::string_utils::is_decimal_digit;
using std::error_code;
using std::get;
using std::make_pair;
using std::monostate;
using std::pair;
using std::string;
using std::string_view;
using std::variant;

namespace clp::regex_utils {

/**
 * Class for storing regex translation config, states, capture group and quantifier information.
 */
class TranslatorState {
public:
    enum class RegexPatternState : uint8_t {
        // The initial state, where characters have no special meanings and are treated literally.
        NORMAL = 0,
        // Encountered a period `.`. Expecting wildcard expression.
        DOT,
        // Encountered a backslash `\`, used to suppress special meanings of regex meta characters.
        ESCAPED,
        // Enclosed by parenthesis `()`, used to specify a capture group.
        GROUP,
        // Encountered a backslash `\` in the capture group.
        GROUPESCAPED,
        // Enclosed by square brackets `[]`, used to specify a character set.
        CHARSET,
        // Encountered a backslash `\` in the character set..
        CHARSETESCAPED,
        // Enclosed by curly brackets `{}`, used to specify a quantity to repeat.
        QUANTIFIER,
        // Encountered a dollar sign `$`, meaning the regex string has reached the end anchor.
        END,
    };

    // Constructor
    TranslatorState(RegexToWildcardTranslatorConfig const& config, string_view regex_str)
            : m_config(config),
              m_it(regex_str.begin()) {}

    // Getters
    [[nodiscard]] auto get_config() const -> RegexToWildcardTranslatorConfig const& {
        return m_config;
    }

    [[nodiscard]] auto get_state() const -> RegexPatternState const& { return m_state; }

    [[nodiscard]] auto get_marked_iterator() const -> string_view::const_iterator const& {
        return m_it;
    }

    [[nodiscard]] auto get_preceding_token(
    ) const -> BOOST_OUTCOME_V2_NAMESPACE::std_result<string>;
    [[nodiscard]] auto get_quantifier() const -> BOOST_OUTCOME_V2_NAMESPACE::std_result<size_t>;

    [[nodiscard]] auto get_quantifier_as_str() const -> string { return m_quantifier_str; }

    [[nodiscard]] auto quantifier_number_start() const -> bool {
        return m_quantifier_str.empty() || ',' == m_quantifier_str.back();
    }

    // Setters
    void set_next_state(RegexPatternState const& state) { m_state = state; }

    void mark_iterator(string_view::const_iterator const& it) { m_it = it; }

    void invalidate_preceding_token() { m_preceding_token = monostate{}; }

    void set_preceding_token(char ch) { m_preceding_token = ch; }

    void set_preceding_token(string const& s) { m_preceding_token = s; }

    void reset_quantifiers() {
        m_quantifier = size_t{0};
        m_quantifier_str.clear();
    }

    void add_to_quantifier(char ch);

    void switch_to_second_quantifier() {
        m_quantifier = make_pair(get<size_t>(m_quantifier), 0);
        m_quantifier_str += ',';
    }

    void inc_nested_group_count() { ++m_nested_group_count; }

    [[nodiscard]] auto dec_nested_group_count() -> BOOST_OUTCOME_V2_NAMESPACE::std_result<size_t>;

private:
    // Variables
    RegexToWildcardTranslatorConfig m_config;
    RegexPatternState m_state = RegexPatternState::NORMAL;
    string_view::const_iterator m_it;
    variant<monostate, char, string> m_preceding_token;
    variant<size_t, pair<size_t, size_t>> m_quantifier;
    string m_quantifier_str;
    size_t m_nested_group_count = 0;
};

auto TranslatorState::get_preceding_token(
) const -> BOOST_OUTCOME_V2_NAMESPACE::std_result<string> {
    switch (m_preceding_token.index()) {
        case 0:
            return ErrorCode::TokenUnquantifiable;
        case 1:
            return string{get<char>(m_preceding_token)};
        case 2:
            return get<string>(m_preceding_token);
        default:
            return ErrorCode::IllegalState;
    }
}

auto TranslatorState::get_quantifier() const -> BOOST_OUTCOME_V2_NAMESPACE::std_result<size_t> {
    switch (m_quantifier.index()) {
        case 0:
            return get<size_t>(m_quantifier);
        case 1:
            // Maybe we can support a ranged pair of quantifiers in the future
            return ErrorCode::UnsupportedQuantifier;
        default:
            return ErrorCode::IllegalState;
    }
}

void TranslatorState::add_to_quantifier(char ch) {
    int const num{ch - '0'};
    int const base = 10;
    switch (m_quantifier.index()) {
        case 0:
            m_quantifier = get<0>(m_quantifier) * base + num;
            break;
        case 1:
            get<1>(m_quantifier).second = get<1>(m_quantifier).second * base + num;
            break;
        default:
            break;
    }
    m_quantifier_str += ch;
}

auto TranslatorState::dec_nested_group_count() -> BOOST_OUTCOME_V2_NAMESPACE::std_result<size_t> {
    if (0 == m_nested_group_count) {
        return ErrorCode::UnmatchedParenthesis;
    }
    --m_nested_group_count;
    return m_nested_group_count;
}

// State transition functions common signature
// typedef [[nodiscard]] auto
// StateTransitionFunc(TranslatorState&, string_view::const_iterator&, string&) -> error_code;

using StateTransitionFunc
        = auto(TranslatorState&, string_view::const_iterator&, string&) -> error_code;

// State transition functions
[[nodiscard]] StateTransitionFunc normal_state_transition;
[[nodiscard]] StateTransitionFunc dot_state_transition;
[[nodiscard]] StateTransitionFunc escaped_state_transition;
[[nodiscard]] StateTransitionFunc group_state_transition;
[[nodiscard]] StateTransitionFunc group_escaped_state_transition;
[[nodiscard]] StateTransitionFunc charset_state_transition;
[[nodiscard]] StateTransitionFunc charset_escaped_state_transition;
[[nodiscard]] StateTransitionFunc quantifier_state_transition;
[[nodiscard]] StateTransitionFunc end_state_transition;
[[nodiscard]] StateTransitionFunc final_state_cleanup;

// Helper function
void append_incomplete_quantifier_structure(TranslatorState& state, string& wildcard_str);
[[nodiscard]] auto matching_upper_lower_case_char_pair(char ch0, char ch1) -> bool;

// Main API
auto regex_to_wildcard(string_view regex_str) -> BOOST_OUTCOME_V2_NAMESPACE::std_result<string> {
    RegexToWildcardTranslatorConfig const default_config{};
    return regex_to_wildcard(regex_str, default_config);
}

auto regex_to_wildcard(string_view regex_str, RegexToWildcardTranslatorConfig const& config)
        -> BOOST_OUTCOME_V2_NAMESPACE::std_result<string> {
    if (regex_str.empty()) {
        return string();
    }

    // Initialize translation state, scan position, and return string
    TranslatorState state{config, regex_str};
    string_view::const_iterator it = regex_str.cbegin();
    string wildcard_str;

    // If there is no starting anchor character, append multichar wildcard prefix
    if (cRegexStartAnchor == *it) {
        if (config.allow_anchors()) {
            ++it;
        } else {
            return ErrorCode::Caret;
        }
    } else if (config.add_prefix_suffix_wildcards()) {
        wildcard_str += cZeroOrMoreCharsWildcard;
    }

    error_code ec{};
    while (it != regex_str.end()) {
        switch (state.get_state()) {
            case TranslatorState::RegexPatternState::NORMAL:
                ec = normal_state_transition(state, it, wildcard_str);
                break;
            case TranslatorState::RegexPatternState::DOT:
                ec = dot_state_transition(state, it, wildcard_str);
                break;
            case TranslatorState::RegexPatternState::ESCAPED:
                ec = escaped_state_transition(state, it, wildcard_str);
                break;
            case TranslatorState::RegexPatternState::GROUP:
                ec = group_state_transition(state, it, wildcard_str);
                break;
            case TranslatorState::RegexPatternState::GROUPESCAPED:
                ec = group_escaped_state_transition(state, it, wildcard_str);
                break;
            case TranslatorState::RegexPatternState::CHARSET:
                ec = charset_state_transition(state, it, wildcard_str);
                break;
            case TranslatorState::RegexPatternState::CHARSETESCAPED:
                ec = charset_escaped_state_transition(state, it, wildcard_str);
                break;
            case TranslatorState::RegexPatternState::QUANTIFIER:
                ec = quantifier_state_transition(state, it, wildcard_str);
                break;
            case TranslatorState::RegexPatternState::END:
                ec = end_state_transition(state, it, wildcard_str);
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
    ec = final_state_cleanup(state, it, wildcard_str);
    if (ec) {
        return ec;
    }

    return wildcard_str;
}

auto normal_state_transition(
        TranslatorState& state,
        string_view::const_iterator& it,
        string& wildcard_str
) -> error_code {
    char const ch = *it;
    auto const& config = state.get_config();
    switch (ch) {
        case '.':
            state.set_next_state(TranslatorState::RegexPatternState::DOT);
            break;
        case cEscapeChar:
            state.set_next_state(TranslatorState::RegexPatternState::ESCAPED);
            break;
        case '(':
            state.inc_nested_group_count();
            state.mark_iterator(it + 1);  // Mark the beginning of group expression
            state.set_next_state(TranslatorState::RegexPatternState::GROUP);
            break;
        case '[':
            state.mark_iterator(it + 1);  // Mark the beginning of charset expression
            state.set_next_state(TranslatorState::RegexPatternState::CHARSET);
            break;
        case '{':
            state.reset_quantifiers();
            state.set_next_state(TranslatorState::RegexPatternState::QUANTIFIER);
            break;
        case cRegexEndAnchor:
            if (!config.allow_anchors()) {
                return ErrorCode::Dollar;
            }
            state.set_next_state(TranslatorState::RegexPatternState::END);
            break;
        case '*':
            return ErrorCode::Star;
        case '+':
            return ErrorCode::Plus;
        case '?':
            return ErrorCode::Question;
        case '|':
            return ErrorCode::Pipe;
        case cRegexStartAnchor:
            return ErrorCode::Caret;
        case ')':
            return ErrorCode::UnmatchedParenthesis;
        default:
            wildcard_str += ch;
            state.set_preceding_token(ch);
            break;
    }
    return ErrorCode::Success;
}

auto dot_state_transition(
        TranslatorState& state,
        string_view::const_iterator& it,
        string& wildcard_str
) -> error_code {
    switch (*it) {
        case '*':
            // .* gets translated to *
            wildcard_str += cZeroOrMoreCharsWildcard;
            state.invalidate_preceding_token();
            break;
        case '+':
            // .+ gets translated to ?*
            wildcard_str = wildcard_str + cSingleCharWildcard + cZeroOrMoreCharsWildcard;
            state.invalidate_preceding_token();
            break;
        default:
            // . gets translated to ?
            wildcard_str += cSingleCharWildcard;
            state.set_preceding_token(cSingleCharWildcard);
            // Backtrack the scan by one position to handle the current char in the next iteration.
            --it;
            break;
    }
    state.set_next_state(TranslatorState::RegexPatternState::NORMAL);
    return ErrorCode::Success;
}

auto escaped_state_transition(
        TranslatorState& state,
        string_view::const_iterator& it,
        string& wildcard_str
) -> error_code {
    char const ch = *it;
    if (!cRegexEscapeSeqAcceptedMetaChars.at(ch)) {
        return ErrorCode::DisallowedEscapeSequence;
    }
    if (cRegexEscapeSeqWildcardOnlyMetaChars.at(ch)) {
        // Need to keep the backslash for characters that are special in the wildcard syntax too
        string const escape_seq = string{cEscapeChar} + ch;
        wildcard_str += escape_seq;
        state.set_preceding_token(escape_seq);
    } else {
        wildcard_str += ch;
        state.set_preceding_token(ch);
    }
    state.set_next_state(TranslatorState::RegexPatternState::NORMAL);
    return ErrorCode::Success;
}

auto group_state_transition(
        TranslatorState& state,
        string_view::const_iterator& it,
        string& wildcard_str
) -> error_code {
    char const ch = *it;
    if (cEscapeChar == ch) {
        state.set_next_state(TranslatorState::RegexPatternState::GROUPESCAPED);
        return ErrorCode::Success;
    }
    // TODO: make the group unrolling iterative
    if ('(' == ch) {
        state.inc_nested_group_count();
        return ErrorCode::Success;
    }
    if (')' != ch) {
        return ErrorCode::Success;
    }
    auto num_nested_group = state.dec_nested_group_count();
    if (num_nested_group.has_error()) {
        return num_nested_group.error();
    }
    if (num_nested_group.value() > 0) {
        // Still within nested group
        return ErrorCode::Success;
    }

    // End of group: translate the captured group expression.
    // capture group should not enable anchors or prefix/suffix wildcards.
    string const captured_group(state.get_marked_iterator(), it);
    auto config{state.get_config()};
    config.set_allow_anchors(false);
    config.set_add_prefix_suffix_wildcards(false);

    // Perform translation
    auto translated_group = regex_to_wildcard(captured_group, config);
    if (translated_group.has_error()) {
        return translated_group.error();
    }

    wildcard_str += translated_group.value();
    state.set_preceding_token(translated_group.value());
    state.set_next_state(TranslatorState::RegexPatternState::NORMAL);
    return ErrorCode::Success;
}

auto group_escaped_state_transition(
        TranslatorState& state,
        string_view::const_iterator& /*it*/,
        string& /*wildcard_str*/
) -> error_code {
    // Defer the handling of escape sequences to entire capture group translation.
    state.set_next_state(TranslatorState::RegexPatternState::GROUP);
    return ErrorCode::Success;
}

auto charset_state_transition(
        TranslatorState& state,
        string_view::const_iterator& it,
        string& wildcard_str
) -> error_code {
    char const ch = *it;
    string_view::const_iterator const& charset_start = state.get_marked_iterator();
    size_t const charset_len = it - charset_start;
    if (cEscapeChar == ch) {
        state.set_next_state(TranslatorState::RegexPatternState::CHARSETESCAPED);
        return ErrorCode::Success;
    }
    if (charset_len > 2) {
        // Short circuit: the currently accepted charset is at most 2-char long.
        return ErrorCode::UnsupportedCharsets;
    }
    if (']' != ch) {
        return ErrorCode::Success;
    }
    if (0 == charset_len) {
        // Empty charset
        return ErrorCode::UnsupportedCharsets;
    }

    // End of charset: perform analysis on accepted charset patterns.
    char const ch0 = *charset_start;
    char const ch1 = *(charset_start + 1);
    auto config{state.get_config()};
    char parsed_char{};

    if (1 == charset_len) {
        if (cCharsetNegate == ch0 || cEscapeChar == ch0) {
            return ErrorCode::UnsupportedCharsets;
        }
        parsed_char = ch0;
    } else {  // 2 == charset_len
        if (cEscapeChar == ch0 && cRegexCharsetEscapeSeqMetaChars.at(ch1)) {
            // 2-char escape sequence
            parsed_char = ch1;
        } else if (config.case_insensitive_wildcard()
                   && matching_upper_lower_case_char_pair(ch0, ch1))
        {
            // case-insensitive patterns like [aA] [Bb] etc.
            parsed_char = ch0 > ch1 ? ch0 : ch1;  // Get the lower case char
        } else {
            return ErrorCode::UnsupportedCharsets;
        }
    }

    // Add the parsed character to the string
    if (cRegexEscapeSeqWildcardOnlyMetaChars.at(parsed_char)) {
        auto escaped_char = string{cEscapeChar} + parsed_char;
        wildcard_str += escaped_char;
        state.set_preceding_token(escaped_char);
    } else {
        wildcard_str += parsed_char;
        state.set_preceding_token(parsed_char);
    }
    state.set_next_state(TranslatorState::RegexPatternState::NORMAL);
    return ErrorCode::Success;
}

auto matching_upper_lower_case_char_pair(char ch0, char ch1) -> bool {
    int const upper_lower_case_ascii_offset = 'a' - 'A';
    return (is_alphabet(ch0) && is_alphabet(ch1)
            && ((ch0 - ch1 == upper_lower_case_ascii_offset)
                || (ch1 - ch0 == upper_lower_case_ascii_offset)));
}

auto charset_escaped_state_transition(
        TranslatorState& state,
        string_view::const_iterator& /*it*/,
        string& /*wildcard_str*/
) -> error_code {
    // Defer the handling of escape sequences to entire character set analysis..
    state.set_next_state(TranslatorState::RegexPatternState::CHARSET);
    return ErrorCode::Success;
}

auto quantifier_state_transition(
        TranslatorState& state,
        string_view::const_iterator& it,
        string& wildcard_str
) -> error_code {
    char const ch = *it;
    if ('-' == ch && state.quantifier_number_start()) {
        // Disallow negative quantifiers
        return ErrorCode::UnsupportedQuantifier;
    }
    if (',' == ch) {
        // Expecting a pair of quantifiers
        state.switch_to_second_quantifier();
    } else if (is_decimal_digit(ch)) {
        // Is a regular decimal digit
        state.add_to_quantifier(ch);
    } else if ('}' != ch) {
        // Invalid quantifier syntax. In such case, the special meaning of `(` is suppressed.
        // So far we've only seen opening bracket/digits/comma, so append directly.
        append_incomplete_quantifier_structure(state, wildcard_str);
        // Backtrack the scan by one position to handle the current char in the next iteration.
        --it;
        state.set_next_state(TranslatorState::RegexPatternState::NORMAL);
    } else {
        // Quantifier expression complete. Perform repetition
        auto quantifier = state.get_quantifier();
        if (quantifier.has_error()) {
            return quantifier.error();
        }
        auto prev_token = state.get_preceding_token();
        if (prev_token.has_error()) {
            return prev_token.error();
        }

        size_t const q_val = quantifier.value();
        string const token = prev_token.value();
        if (0 == q_val) {
            // Zero repetition removes the token from the string
            wildcard_str.erase(wildcard_str.length() - token.length());
        } else {
            // Repeat the token for n-1 times
            for (size_t i{1}; i < q_val; ++i) {
                wildcard_str += token;
            }
        }
        // Compound repetition is not allowed.
        state.invalidate_preceding_token();
        state.set_next_state(TranslatorState::RegexPatternState::NORMAL);
    }
    return ErrorCode::Success;
}

auto end_state_transition(
        TranslatorState& /*state*/,
        string_view::const_iterator& it,
        string& /*wildcard_str*/
) -> error_code {
    if (cRegexEndAnchor != *it) {
        return ErrorCode::Dollar;
    }
    return ErrorCode::Success;
}

auto final_state_cleanup(
        TranslatorState& state,
        string_view::const_iterator& /*it*/,
        string& wildcard_str
) -> error_code {
    switch (state.get_state()) {
        case TranslatorState::RegexPatternState::DOT:
            // The last character is a single `.`, without the possibility of becoming a
            // multichar wildcard
            wildcard_str += cSingleCharWildcard;
            break;
        case TranslatorState::RegexPatternState::ESCAPED:
            return ErrorCode::DisallowedEscapeSequence;
        case TranslatorState::RegexPatternState::GROUP:
        case TranslatorState::RegexPatternState::GROUPESCAPED:
            return ErrorCode::UnmatchedParenthesis;
        case TranslatorState::RegexPatternState::CHARSET:
            return ErrorCode::IncompleteCharsetStructure;
        case TranslatorState::RegexPatternState::QUANTIFIER:
            append_incomplete_quantifier_structure(state, wildcard_str);
            break;
        default:
            break;
    }

    auto const& config = state.get_config();
    if (TranslatorState::RegexPatternState::END != state.get_state()
        && config.add_prefix_suffix_wildcards())
    {
        wildcard_str += cZeroOrMoreCharsWildcard;
    }
    return ErrorCode::Success;
}

void append_incomplete_quantifier_structure(TranslatorState& state, string& wildcard_str) {
    // Invalid quantifier syntax. So far we've only seen digits/comma so append directly.
    string const invalid_quantifier_str = string{'{'} + state.get_quantifier_as_str();
    wildcard_str += invalid_quantifier_str;
    state.set_preceding_token(invalid_quantifier_str.back());
}

}  // namespace clp::regex_utils
