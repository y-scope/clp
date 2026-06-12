#ifndef CLP_S_SEARCH_PROJECTION_HPP
#define CLP_S_SEARCH_PROJECTION_HPP

#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include <clp_s/ErrorCode.hpp>
#include <clp_s/SchemaTree.hpp>
#include <clp_s/search/ast/ColumnDescriptor.hpp>
#include <clp_s/TraceableException.hpp>

namespace clp_s::search {
/**
 * This class describes the set of columns that should be included in the projected results.
 *
 * After adding columns and before calling matches_node the caller is responsible for calling
 * resolve_columns.
 */
class Projection {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    enum class Mode : uint8_t {
        ReturnAllColumns,
        ReturnSelectedColumns
    };

    enum class NodeProjection : uint8_t {
        Default = 1,
        Decomposed = 2,
        Shape = 4,
    };

    using node_projection_mask_t = std::underlying_type_t<NodeProjection>;

    // Constructors
    /**
     * Constructs a Projection object with the specified mode and column handling behavior.
     *
     * @param mode Projection mode that determines how expressions are projected.
     * @param allow_duplicate_columns Whether duplicate column descriptors are permitted in the
     * projection.
     */
    explicit Projection(Mode mode, bool allow_duplicate_columns = false)
            : m_projection_mode{mode},
              m_allow_duplicate_columns{allow_duplicate_columns} {}

    // Methods
    /**
     * Adds a column to the set of columns that should be included in the projected results
     * @param column
     * @param is_decomposed Whether this column descriptor requests decomposed output (shape +
     * leaf values).
     * @param is_shape Whether this column descriptor requests only the shape.
     * @throws OperationFailed if `column` contains a wildcard
     * @throws OperationFailed if this instance of Projection is in mode ReturnAllColumns
     * @throws OperationFailed if `column` is identical to a previously added column
     * @throws std::runtime_error if both `is_decomposed` and `is_shape` are true
     */
    auto add_column(
            std::shared_ptr<ast::ColumnDescriptor> column,
            bool is_decomposed = false,
            bool is_shape = false
    ) -> void;

    /**
     * Gets the current projection mode.
     * @return The projection mode.
     */
    [[nodiscard]] auto get_projection_mode() const -> Mode { return m_projection_mode; }

    /**
     * Checks whether a given node has a specific projection mode set.
     * @param node_id The schema node ID to check.
     * @param projection The projection mode(s) to test.
     * @return true if the node has the requested projection mode.
     */
    [[nodiscard]] auto is_projected_as(SchemaNode::id_t node_id, NodeProjection projection) const
            -> bool;

    /**
     * Checks whether a given node has any projection mode set (default, decomposed, or shape).
     * @param node_id The schema node ID to check.
     * @return true if any projection mode is active for the node.
     */
    [[nodiscard]] auto has_any_projection(SchemaNode::id_t node_id) const -> bool;

    /**
     * Resolves all columns for the purpose of projection. This key resolution implementation is
     * more limited than the one in schema matching. In particular, this version of key resolution
     * only allows resolving keys that do not contain wildcards and does not allow resolving to
     * objects within arrays.
     *
     * Note: we could try to generalize column resolution code/move it to the schema tree. It is
     * probably best to write a simpler version dedicated to projection for now since types are
     * leaf-only. The type-per-token idea solves this problem (in the absence of wildcards).
     *
     * @param tree
     */
    auto resolve_columns(SchemaTree const& tree) -> void;

    /**
     * Checks whether a column corresponding to given leaf node should be included in the output
     * @param node_id
     * @return true if the column should be included in the output, false otherwise
     */
    [[nodiscard]] auto matches_node(SchemaNode::id_t node_id) const -> bool {
        return Mode::ReturnAllColumns == m_projection_mode || m_matching_nodes.contains(node_id);
    }

private:
    // Types
    enum class OutputType : uint8_t {
        Default,
        Decomposed,
        Shape
    };

    struct TargetColumn {
        // Constructors
        TargetColumn(
                std::shared_ptr<ast::ColumnDescriptor> column,
                OutputType output_type,
                std::vector<SchemaNode::id_t> matched_nodes
        )
                : m_column(std::move(column)),
                  m_output_type(output_type),
                  m_matched_nodes(std::move(matched_nodes)) {}

        // Data members
        std::shared_ptr<ast::ColumnDescriptor> m_column;
        OutputType m_output_type;
        std::vector<SchemaNode::id_t> m_matched_nodes;
    };

    // Methods
    /**
     * Resolves an individual column as described by the `resolve_columns` method.
     * @param tree
     * @param column
     * @return The list of schema node IDs matched by the column descriptor.
     */
    [[nodiscard]] auto resolve_column(SchemaTree const& tree, ast::ColumnDescriptor& column)
            -> std::vector<SchemaNode::id_t>;

    /**
     * Records a projection mode for a schema node by OR-ing it into the node's projection bitmask.
     * @param node_id
     * @param projection
     */
    auto add_projection(SchemaNode::id_t node_id, NodeProjection projection) -> void;

    /**
     * For a resolved column's matched nodes, identifies structural nodes (LogMessage / ParentRule)
     * and records their projection mode. Returns whether any structural node was found.
     * @param tree
     * @param matched_nodes The nodes matched by the column descriptor.
     * @param output_type The output type for this column.
     * @return true if at least one matched node was a structural container.
     */
    [[nodiscard]] auto collect_structural_projections(
            SchemaTree const& tree,
            std::vector<SchemaNode::id_t> const& matched_nodes,
            OutputType output_type
    ) -> bool;

    // Data members
    std::vector<TargetColumn> m_columns;
    absl::flat_hash_set<SchemaNode::id_t> m_matching_nodes;
    absl::flat_hash_map<SchemaNode::id_t, node_projection_mask_t> m_node_projections;
    Mode m_projection_mode{Mode::ReturnAllColumns};
    bool m_allow_duplicate_columns{false};
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_PROJECTION_HPP
