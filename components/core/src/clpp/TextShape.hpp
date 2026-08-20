#ifndef CLPP_TEXTSHAPE_HPP
#define CLPP_TEXTSHAPE_HPP

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <log_surgeon/log_surgeon.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clpp/Defs.hpp>
#include <clpp/ErrorCode.hpp>
#include <clpp/ParentRuleShapes.hpp>

namespace clpp {
/**
 * Requirement of the TextShape storage type. Requires the type to expose a contiguous character
 * buffer through `data()` and `size()` (similar to `std::string` and `std::string_view`).
 */
template <typename T>
concept TextShapeStorageReq = requires(T const& t) {
    { t.data() } -> std::same_as<char const*>;
    { t.size() } -> std::same_as<std::size_t>;
};

/**
 * Requirement for a storage type capable of building a TextShape.
 */
template <typename T>
concept TextShapeBuilderReq
        = TextShapeStorageReq<T> && requires(T& t, std::string_view sv, std::size_t n, char c) {
              { t.reserve(n) } -> std::same_as<void>;
              { t.append(sv) } -> std::same_as<T&>;
              { t.push_back(c) } -> std::same_as<void>;
          };

/**
 * A wrapper around a text shape (i.e. a log shape or parent rule shape) that provides typed methods
 * for building, querying, and narrowing the shape.
 *
 * A text shape is a sequence of segments made of escaped literal text ('%' characters are doubled
 * '%%') and leaf rule placeholders (delimited by lone '%' characters, e.g.
 * `%qualified-leaf-rule-name%`).
 *
 * @tparam Storage The underlying string storage type.
 */
template <TextShapeStorageReq Storage>
class TextShape {
public:
    // Types
    /**
     * A text shape segment.
     *
     * `Literal`: escaped literal text.
     * `Placeholder`: the qualified leaf rule name without the surrounding `%` delimiters.
     */
    struct Segment {
        enum class Type : uint8_t {
            Literal,
            Placeholder
        };
        Type type;
        std::string_view text;
    };

    // Static methods
    /**
     * Reverses the escaping applied to a literal text segment (replaces every '%%' with '%').
     *
     * This function is only safe when used with a single literal segment. It cannot distinguish
     * whether '%%' is an escaped literal or the closing+opening delimiters of two adjacent
     * placeholders.
     *
     * @param shape The literal segment to unescape.
     * @return An unescaped copy of the literal segment.
     */
    [[nodiscard]] static auto unescape_literal_text(std::string_view literal_segment)
            -> std::string {
        std::string result;
        result.reserve(literal_segment.size());
        for (size_t i{0}; i < literal_segment.size();) {
            if ('%' == literal_segment.at(i) && i + 1 < literal_segment.size()
                && '%' == literal_segment.at(i + 1))
            {
                result += '%';
                i += 2;
            } else {
                result += literal_segment.at(i);
                ++i;
            }
        }
        return result;
    }

    // Constructors
    TextShape() = default;

    explicit TextShape(Storage storage) : m_storage(std::move(storage)) {}

    /**
     * Constructs an empty TextShape with pre-allocated capacity.
     *
     * @param capacity The number of bytes to reserve in the underlying storage.
     */
    explicit TextShape(size_t capacity)
    requires TextShapeBuilderReq<Storage>
            : m_storage{} {
        m_storage.reserve(capacity);
    }

    // Methods
    [[nodiscard]] auto empty() const noexcept -> bool { return 0 == m_storage.size(); }

    [[nodiscard]] auto view() const -> std::string_view {
        return {m_storage.data(), m_storage.size()};
    }

    /**
     * Splits the shape into its segments.
     *
     * @return The shape's `Segment`s in document order.
     */
    [[nodiscard]] auto segments() const -> std::vector<Segment> {
        std::vector<Segment> result;
        auto const shape{view()};
        for (size_t pos{0}; pos < shape.size();) {
            auto const open{find_placeholder_opening(pos)};
            if (std::string_view::npos == open) {
                result.emplace_back(Segment::Type::Literal, shape.substr(pos));
                return result;
            }
            if (open > pos) {
                result.emplace_back(Segment::Type::Literal, shape.substr(pos, open - pos));
            }
            // The closing placeholder delimiter is always the next '%' after the opening one as a
            // column name cannot contain a delimiter.
            auto const close{shape.find('%', open + 1)};
            if (std::string_view::npos == close) {
                result.emplace_back(Segment::Type::Literal, shape.substr(open));
                return result;
            }
            result.emplace_back(
                    Segment::Type::Placeholder,
                    shape.substr(open + 1, close - open - 1)
            );
            pos = close + 1;
        }
        return result;
    }

    /**
     * Builds the parent rule shapes for this text shape.
     *
     * Parent rule `log_surgeon::Match` ranges are positions into the log message, which is not
     * escaped. To build the parent rule shapes we map each parent rule's [start, end) range into
     * the escaped shape's positions and collect them into the returned ParentRuleShapes.
     *
     * Because placeholders are used for leaf rule matches and the leaf matches appear in document
     * order within the event's matches, the i-th leaf corresponds to the i-th placeholder in the
     * shape.
     *
     * @param event The log event containing all matches and the message text.
     * @return A result containing the parent rule shapes or an error code indicating the failure:
     * - ClppErrorCodeEnum::Corrupt if there is a mismatch between segments and leaf matches.
     */
    [[nodiscard]] auto build_parent_rule_shapes(log_surgeon::LogEvent const& event) const
            -> ystdlib::error_handling::Result<ParentRuleShapes> {
        std::vector<std::pair<size_t, size_t>> message_to_shape_positions;
        size_t shape_pos{0};
        size_t leaf_match_idx{0};
        for (auto const& segment : segments()) {
            switch (segment.type) {
                case Segment::Type::Literal:
                    shape_pos += segment.text.size();
                    break;
                case Segment::Type::Placeholder: {
                    auto const leaf_match{event.get_leaf_match(leaf_match_idx)};
                    if (false == leaf_match.has_value()) {
                        return ClppErrorCode{ClppErrorCodeEnum::Corrupt};
                    }
                    message_to_shape_positions.emplace_back(leaf_match->range.start, shape_pos);
                    message_to_shape_positions.emplace_back(
                            leaf_match->range.end,
                            shape_pos + segment.text.size() + 2
                    );
                    ++leaf_match_idx;
                    shape_pos += segment.text.size() + 2;
                    break;
                }
            }
        }

        auto const message{event.get_message()};
        auto map_message_to_shape_pos{[&](size_t message_pos) -> size_t {
            std::pair<size_t, size_t> preceding{0, 0};
            for (auto const& pos : message_to_shape_positions) {
                if (pos.first > message_pos) {
                    break;
                }
                preceding = pos;
            }
            auto const message_segment{
                    message.substr(preceding.first, message_pos - preceding.first)
            };
            return preceding.second + (message_pos - preceding.first)
                   + static_cast<size_t>(std::ranges::count(message_segment, '%'));
        }};

        ParentRuleShapes parent_shapes;
        for (auto const& match : event.get_all_matches()) {
            if (match.is_leaf) {
                continue;
            }
            auto const start{map_message_to_shape_pos(match.range.start)};
            auto const end{map_message_to_shape_pos(match.range.end)};
            parent_shapes.emplace_parent_rule_shape(
                    match.get_fully_qualified_name(),
                    start,
                    end - start
            );
        }
        return parent_shapes;
    }

    /**
     * Return a parent rule shape narrowed from this shape.
     *
     * @param parent_rule_shapes All parent rule shapes contained in this shape.
     * @param parent_rule_column_name The qualified name of the parent rule to narrow to.
     * @return A TextShape view of the parent rule, or an empty view on failure.
     */
    [[nodiscard]] auto narrow_to_parent_rule(
            clpp::ParentRuleShapes const& parent_rule_shapes,
            std::string_view parent_rule_column_name
    ) const -> TextShape<std::string_view> {
        auto const log_shape{view()};
        for (auto const& match : parent_rule_shapes.get()) {
            if (match.m_name == parent_rule_column_name) {
                if (match.m_start < log_shape.size()
                    && match.m_start + match.m_size <= log_shape.size())
                {
                    return TextShape<std::string_view>{
                            log_shape.substr(match.m_start, match.m_size)
                    };
                }
                return {};
            }
        }
        return {};
    }

    auto escape_and_append(std::string_view text) -> void
    requires TextShapeBuilderReq<Storage>
    {
        for (auto const c : text) {
            m_storage.push_back(c);
            if ('%' == c) {
                m_storage.push_back('%');
            }
        }
    }

    auto append_placeholder(std::string_view column_name) -> void
    requires TextShapeBuilderReq<Storage>
    {
        m_storage.reserve(m_storage.size() + column_name.size() + 2);
        m_storage.push_back('%');
        m_storage.append(column_name);
        m_storage.push_back('%');
    }

private:
    // Methods
    /**
     * Finds the next opening placeholder delimiter ('%'), starting from `pos`. Escaped literals
     * ('%%') are skipped.
     *
     * This function is only safe when starting from literal text and cannot be used to find a
     * closing delimiter. It cannot distinguish whether '%%' is an escaped literal or the
     * closing+opening delimiters of two adjacent placeholders.
     *
     * @param shape The escaped shape text to scan.
     * @param pos The starting position for the scan.
     * @return The position of the next opening placeholder delimiter '%', or std::string_view::npos
     * if not found.
     */
    [[nodiscard]] auto find_placeholder_opening(size_t pos) const -> size_t {
        auto const shape{view()};
        while (pos < shape.size()) {
            auto const pct{shape.find('%', pos)};
            if (std::string_view::npos == pct) {
                return std::string_view::npos;
            }
            if (pct + 1 < shape.size() && '%' == shape.at(pct + 1)) {
                pos = pct + 2;
                continue;
            }
            return pct;
        }
        return std::string_view::npos;
    }

    // Data members
    Storage m_storage;
};
}  // namespace clpp

#endif  // CLPP_TEXTSHAPE_HPP
