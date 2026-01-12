#ifndef CLP_S_EXPERIMENTAL_HELPERS_HPP
#define CLP_S_EXPERIMENTAL_HELPERS_HPP

#include <cstddef>
#include <vector>

#include <log_surgeon/finite_automata/Capture.hpp>
#include <log_surgeon/finite_automata/PrefixTree.hpp>
#include <log_surgeon/LogEvent.hpp>
#include <log_surgeon/Token.hpp>
#include <ystdlib/error_handling/Result.hpp>

/**
 * This file contains helper classes and functions to build on the Log Surgeon API for what is
 * necessary in the experimental prototype. This implies that everything in this file should be
 * treated as unstable and not used outside the experimental flag.
 * Long term most of this functionality should be moved to Log Surgeon, but in the short term it is
 * easier to iterate directly at the case while we work on stabilizing the APIs.
 */

namespace clp_s {
/**
 * Stores the start and end position for a capture group's match within a `log_surgeon::Token`.
 */
struct CapturePosition {
    using Pos = log_surgeon::finite_automata::PrefixTree::position_t;

    CapturePosition(Pos start, Pos end) : m_start(start), m_end(end) {}

    Pos m_start;
    Pos m_end;
};

/**
 * Retrieves the position of `capture` within `root_var`. `root_var` must be the root parent
 * variable containing `capture`.
 * @param event The log event containing the `root_var` and `capture`.
 * @param root_var The parent log surgeon schema variable for `capture`.
 * @param capture The capture group match.
 * @return A result containing a `CapturePosition` on success, or an error code indicating the
 * failure:
 * - ClpsErrorCodeEnum::Failure if the capture's positions are invalid.
 */
auto get_capture_positions(
        log_surgeon::LogEvent const& event,
        log_surgeon::Token const& root_var,
        log_surgeon::finite_automata::Capture const* const& capture
) -> ystdlib::error_handling::Result<CapturePosition>;

/**
 * Check if the `idx` capture group in `captures` is a leaf capture group.
 * @param event The log event containing the `root_var` and the `captures`.
 * @param root_var The root log surgeon variable token for the `captures`.
 * @param captures The capture group matches within `root_var`.
 * @param idx
 * @param cur_end_pos The end position of the current capture group.
 * @return A result containing a `true` if `id` is a leaf capture group (`false` if not), or an
 * error code indicating the failure:
 * - Forwards `get_capture_positions`'s return values on failure.
 */
auto is_leaf_capture(
        log_surgeon::LogEvent const& event,
        log_surgeon::Token const& root_var,
        std::vector<log_surgeon::finite_automata::Capture const*> const& captures,
        size_t idx,
        log_surgeon::finite_automata::PrefixTree::position_t cur_end_pos
) -> ystdlib::error_handling::Result<bool>;

/**
 * Sorts the `captures` within `root_var` by increasing start position, then by decreasing end
 * position.
 * A capture group can only be nested inside another and not span across another. This ensures that
 * the sorted capture groups are ordered by their appear within the text, with a parent capture
 * groups before their children. Therefore, a capture group is a leaf if its end position is less
 * than the end position of the next capture group (or it is the last capture group).
 * @param event The log event containing the `root_var` and the `captures`.
 * @param root_var The root log surgeon variable token for the `captures`.
 * @param captures The capture group matches within `root_var`.
 */
auto sort_capture_ids(
        log_surgeon::LogEvent const& event,
        log_surgeon::Token root_var,
        std::vector<log_surgeon::finite_automata::Capture const*>& captures
) -> void;
}  // namespace clp_s

#endif  // CLP_S_EXPERIMENTAL_HELPERS_HPP
