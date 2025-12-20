#include "experimental_helpers.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <log_surgeon/finite_automata/Capture.hpp>
#include <log_surgeon/finite_automata/PrefixTree.hpp>
#include <log_surgeon/LogEvent.hpp>
#include <log_surgeon/LogParser.hpp>
#include <log_surgeon/Token.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ErrorCode.hpp>

namespace clp_s {
auto get_capture_positions(
        log_surgeon::LogEvent const& event,
        log_surgeon::Token const& root_variable,
        log_surgeon::finite_automata::Capture const* const& capture
) -> ystdlib::error_handling::Result<CapturePosition> {
    auto const& lexer{event.get_log_parser().m_lexer};
    auto const [start_reg_id, end_reg_id]{lexer.get_reg_ids_from_capture(capture)};
    auto const start_positions{root_variable.get_reversed_reg_positions(start_reg_id)};
    auto const end_positions{root_variable.get_reversed_reg_positions(end_reg_id)};
    if (start_positions.empty() || 0 > start_positions[0] || end_positions.empty()
        || 0 > end_positions[0])
    {
        return ClpsErrorCode{ClpsErrorCodeEnum::Failure};
    }
    return {start_positions[0], end_positions[0]};
}

auto is_leaf_capture(
        log_surgeon::LogEvent const& event,
        log_surgeon::Token const& root_variable,
        std::vector<log_surgeon::finite_automata::Capture const*> const& captures,
        size_t idx,
        log_surgeon::finite_automata::PrefixTree::position_t cur_end_pos
) -> ystdlib::error_handling::Result<bool> {
    auto const next_idx{idx + 1};
    if (next_idx == captures.size()) {
        return true;
    }
    auto const& next_capture{captures[next_idx]};
    auto [unused, next_end_pos]{
            YSTDLIB_ERROR_HANDLING_TRYX(get_capture_positions(event, root_variable, next_capture))
    };
    if (next_end_pos > cur_end_pos) {
        return true;
    }
    return false;
}

auto sort_capture_ids(
        log_surgeon::LogEvent const& event,
        log_surgeon::Token root_variable,
        std::vector<log_surgeon::finite_automata::Capture const*>& captures
) -> void {
    std::sort(captures.begin(), captures.end(), [&](auto& a, auto& b) -> bool {
        static constexpr std::string_view cErrorFmt{
                "Could not get positions for capture: {}, error: {} ({})"
        };

        auto const a_pos_result{get_capture_positions(event, root_variable, a)};
        if (a_pos_result.has_error()) {
            throw std::runtime_error(
                    fmt::format(
                            cErrorFmt,
                            a->get_name(),
                            a_pos_result.error().category().name(),
                            a_pos_result.error().message()
                    )
            );
        }
        auto const [a_start_pos, a_end_pos]{a_pos_result.value()};

        auto const b_pos_result{get_capture_positions(event, root_variable, b)};
        if (b_pos_result.has_error()) {
            throw std::runtime_error(
                    fmt::format(
                            cErrorFmt,
                            b->get_name(),
                            b_pos_result.error().category().name(),
                            b_pos_result.error().message()
                    )
            );
        }
        auto const [b_start_pos, b_end_pos]{b_pos_result.value()};

        if (a_start_pos != b_start_pos) {
            return a_start_pos < b_start_pos;
        }
        return a_end_pos > b_end_pos;
    });
}
}  // namespace clp_s
