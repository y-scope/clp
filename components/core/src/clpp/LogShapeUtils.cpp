#include "LogShapeUtils.hpp"

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <vector>

#include <log_surgeon/log_surgeon.hpp>

#include <clpp/ParentRuleShapes.hpp>

namespace clpp {
namespace {
/**
 * A raw-message offset paired with its corresponding escaped-shape offset.
 */
struct RawShapeOffset {
    RawShapeOffset(size_t raw, size_t shape) : raw(raw), shape(shape) {}

    size_t raw;
    size_t shape;
};
}  // namespace

// The closing placeholder delimiter is always the next '%' after the opening one as a column
// name cannot contain a delimiter. Using `find_placeholder_delimiter` to find the closing delimiter
// can incorrectly treat the closing and opening delimiters of adjacent placeholders (e.g.
// `%var%%var%`) as an escaped literal as the two delimiters appear as `%%`.
auto build_parent_rule_shapes(log_surgeon::EventHandle const& event, std::string_view log_shape)
        -> ParentRuleShapes {
    std::vector<RawShapeOffset> raw_shape_offsets{{0, 0}};

    size_t leaf_idx{0};
    size_t pos{0};
    while (pos < log_shape.size()) {
        auto const open{find_placeholder_delimiter(log_shape, pos)};
        if (std::string_view::npos == open) {
            break;
        }
        auto const close{log_shape.find('%', open + 1)};
        if (std::string_view::npos == close) {
            break;
        }
        auto const leaf{event.get_leaf_match(leaf_idx)};
        if (false == leaf.has_value()) {
            break;
        }
        raw_shape_offsets.emplace_back(leaf->range.start, open);
        raw_shape_offsets.emplace_back(leaf->range.end, close + 1);
        ++leaf_idx;
        pos = close + 1;
    }

    auto map{[&](size_t p) -> size_t {
        auto base{raw_shape_offsets.front()};
        for (auto const& offset : raw_shape_offsets) {
            if (offset.raw > p) {
                break;
            }
            base = offset;
        }
        auto const static_text{event.get_message().substr(base.raw, p - base.raw)};
        return base.shape + (p - base.raw)
               + static_cast<size_t>(std::ranges::count(static_text, '%'));
    }};

    ParentRuleShapes parent_shapes;
    for (auto const& match : event.get_all_matches()) {
        if (match.is_leaf) {
            continue;
        }
        auto const start{map(match.range.start)};
        auto const end{map(match.range.end)};
        parent_shapes.emplace_parent_rule_shape(
                match.ffi_pointers.fully_qualified_name.as_cpp_view(),
                start,
                end - start
        );
    }
    return parent_shapes;
}
}  // namespace clpp
