#ifndef CLPP_INTERPRETATION_HPP
#define CLPP_INTERPRETATION_HPP

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <log_surgeon/log_surgeon.hpp>

#include <clpp/TextShape.hpp>

namespace clpp {
/**
 * A query for a single leaf rule match, split into the leaf's qualified rule name, the query
 * string for the leaf's value, and the leaf's position.
 *
 * `m_leaf_position` is the index of the leaf's placeholder among the leaf placeholders of the shape
 * the query was decomposed against, so the i-th leaf query of an interpretation constrains the i-th
 * leaf placeholder. `decompose_by_rule_name` yields positions relative to the parent rule's shape;
 * `decompose_by_log_shapes` yields positions relative to the log shape.
 */
struct LeafQuery {
    // Constructors
    LeafQuery(std::string_view qualified_name, std::string_view match, size_t leaf_position)
            : m_qualified_name(qualified_name),
              m_query(match),
              m_leaf_position(leaf_position) {}

    // Data members
    std::string m_qualified_name;
    std::string m_query;
    size_t m_leaf_position;
};

/**
 * One way an input query can be interpreted against a log shape or parent rule shape, split into
 * the shape query, and the queries for each leaf rule match.
 */
struct Interpretation {
    // Constructors
    Interpretation(TextShape<std::string> shape_query, std::vector<LeafQuery> leaf_queries)
            : m_shape_query(std::move(shape_query)),
              m_leaf_queries(std::move(leaf_queries)) {}

    // Data members
    TextShape<std::string> m_shape_query;
    std::vector<LeafQuery> m_leaf_queries;
};

/**
 * Decomposes `query` against a named log-surgeon rule, returning every interpretation of the query
 * for `rule_name`.
 *
 * @param parser
 * @param query
 * @param rule_name The qualified (dot-separated) log-surgeon rule name.
 * @return A vector of the interpretations for `query` on `rule_name`.
 * @throw Propagates `log_surgeon::Parser::search_by_name`'s exceptions.
 * @throws std::system_error (clpp::ClppErrorCodeEnum::Unsupported) if built without
 * CLP_BUILD_CLPP_DECOMPOSITION.
 */
[[nodiscard]] auto decompose_by_rule_name(
        log_surgeon::Parser& parser,
        std::string_view query,
        std::string_view rule_name
) -> std::vector<Interpretation>;

/**
 * Decomposes `query` against `log_shapes`, returning a list of interpretations for every shape.
 * Currently, shape matching is always case-sensitive. Shapes with interpretations are known to
 * satisfy the query while any shape that cannot satisfy the query will have no interpretations.
 *
 * @param parser
 * @param query
 * @param log_shapes
 * @return A vector of the interpretations for every shape in `log_shapes`. The vector is the same
 * size and order of `log_shapes`, with empty elements for shapes that cannot match the query. Each
 * interpretation is the vector of leaf queries for that interpretation, in document order. An empty
 * vector of leaf queries means the shape matches `query` without constraining any leaf values.
 * @throw Propagates `log_surgeon::Parser::search_by_log_shapes`'s exceptions.
 * @throws std::system_error (clpp::ClppErrorCodeEnum::Unsupported) if built without
 * CLP_BUILD_CLPP_DECOMPOSITION.
 */
[[nodiscard]] auto decompose_by_log_shapes(
        log_surgeon::Parser& parser,
        std::string_view query,
        std::span<std::string_view const> log_shapes
) -> std::vector<std::vector<std::vector<LeafQuery>>>;

/**
 * Splits a qualified (dot-separated) rule name into its segments.
 * @param qualified_name
 * @return The list of rule name segments starting from the root rule.
 */
[[nodiscard]] auto split_qualified_name(std::string_view qualified_name)
        -> std::vector<std::string_view>;
}  // namespace clpp
#endif  // CLPP_INTERPRETATION_HPP
