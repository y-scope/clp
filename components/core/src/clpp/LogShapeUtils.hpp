#ifndef CLPP_LOGSHAPEUTILS_HPP
#define CLPP_LOGSHAPEUTILS_HPP

#include <cstddef>
#include <string>
#include <string_view>

#include <log_surgeon/log_surgeon.hpp>

#include <clpp/ParentRuleShapes.hpp>

namespace clpp {
/**
 * Escape literal '%' as '%%' in static text portions of a log shape template.
 * This ensures the placeholder delimiters (%qualified-name%) are unambiguous.
 *
 * @param text The static text to escape.
 * @return A copy of `text` with every '%' doubled to '%%'.
 */
[[nodiscard]] inline auto escape_shape_text(std::string_view text) -> std::string {
    std::string result;
    result.reserve(text.size());
    for (auto const c : text) {
        result += c;
        if ('%' == c) {
            result += c;
        }
    }
    return result;
}

/**
 * Reverse of escape_shape_text: replace every '%%' with '%'.
 * Useful for getting the original text of a shape.
 *
 * @param text The escaped text to unescape.
 * @return A copy of `text` with every '%%' collapsed back to '%'.
 */
[[nodiscard]] inline auto unescape_shape_text(std::string_view text) -> std::string {
    std::string result;
    result.reserve(text.size());
    for (size_t i{0}; i < text.size();) {
        if ('%' == text.at(i) && i + 1 < text.size() && '%' == text.at(i + 1)) {
            result += '%';
            i += 2;
        } else {
            result += text.at(i);
            ++i;
        }
    }
    return result;
}

/**
 * Find the next placeholder delimiter ('%') in an escaped shape.
 * Skips '%%' (escaped literal percent) sequences, so the only characters this
 * returns are the lone '%' characters that delimit `%qualified-name%` placeholders.
 *
 * @param text The escaped template string to scan.
 * @param pos  The starting position for the scan.
 * @return The position of the next placeholder delimiter '%', or npos if not found.
 */
[[nodiscard]] inline auto find_placeholder_delimiter(std::string_view text, size_t pos) -> size_t {
    while (pos < text.size()) {
        auto const pct{text.find('%', pos)};
        if (std::string_view::npos == pct) {
            return std::string_view::npos;
        }
        if (pct + 1 < text.size() && '%' == text.at(pct + 1)) {
            pos = pct + 2;
            continue;
        }
        return pct;
    }
    return std::string_view::npos;
}

/**
 * Build the parent rule shapes for a log message's escaped shape.
 *
 * Parent rule matches in `event` carry offsets into `event`'s log message, which is not escaped. In
 * order to build the parent rule shapes we map each parent's [start, end) range into the
 * escaped-shape's coordinates and collect them into the returned ParentRuleShapes.
 *
 * Because placeholders are used for leaf rule matches and `event` returns the leaf matches in
 * document order, the i-th leaf corresponds to the i-th placeholder.
 *
 * @param event The log event whose parent matches are mapped.
 * @param log_shape The escaped log shape of `event`'s message.
 * @return The parent rule shapes with ranges in escaped-shape coordinates.
 */
[[nodiscard]] auto
build_parent_rule_shapes(log_surgeon::EventHandle const& event, std::string_view log_shape)
        -> ParentRuleShapes;
}  // namespace clpp

#endif  // CLPP_LOGSHAPEUTILS_HPP
