#include <string>
#include <string_view>

#include "regex_utils/constants.hpp"
#include "regex_utils/regex_utils.hpp"

using std::string;
using std::string_view;

namespace clp::regex_utils {

auto regex_trim_line_anchors(string_view regex_str) -> string {
    string_view::const_iterator left(regex_str.begin());
    string_view::const_iterator right(regex_str.end());

    // Find the position of the first non-caret character
    while (left != right && cRegexStartAnchor == *left) {
        ++left;
    }
    // Backtrack one char to include at least one start anchor, if there was any.
    if (left != regex_str.begin()) {
        --left;
    }

    // Find the position of the last non-dollar-sign character
    while (left != right && cRegexEndAnchor == *(right - 1)) {
        --right;
    }
    if (left != right && right != regex_str.end()) {
        // There was at least one end anchor so we include it by advancing one char
        ++right;
    }

    // If there was more than one end anchor, we need to check if the current end anchor is escaped.
    // If so, it's not a real end anchor, and we need to advance the end position once more to
    // append a real end anchor.
    string trimmed_regex_str(left, right);
    if (right != regex_str.end() && !regex_has_end_anchor(trimmed_regex_str)) {
        trimmed_regex_str += cRegexEndAnchor;
    }
    return trimmed_regex_str;
}

auto regex_has_start_anchor(string_view regex_str) -> bool {
    return !regex_str.empty() && cRegexStartAnchor == regex_str.at(0);
}

auto regex_has_end_anchor(string_view regex_str) -> bool {
    auto it{regex_str.rbegin()};
    if (it == regex_str.rend() || cRegexEndAnchor != *it) {
        return false;
    }

    // Check that ending regex dollar sigh char is unescaped.
    // We need to scan the suffix until we encounter a character that is not an
    // escape char, since escape chars can escape themselves.
    bool escaped{false};
    for (++it; it != regex_str.rend() && cEscapeChar == *it; ++it) {
        escaped = !escaped;
    }
    return !escaped;
}

}  // namespace clp::regex_utils
