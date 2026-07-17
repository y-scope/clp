#ifndef CLP_S_SEARCH_PROJECTION_HPP
#define CLP_S_SEARCH_PROJECTION_HPP

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include <clp_s/ErrorCode.hpp>
#include <clp_s/SchemaTree.hpp>
#include <clp_s/search/ast/ColumnDescriptor.hpp>
#include <clp_s/search/ast/FunctionCall.hpp>
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

    /**
     * Per-schema-node projection mask tracking which output modes are active for a given schema
     * node.
     */
    class NodeMask {
    public:
        // Types
        /**
         * Output modes for a projected schema node.
         * - Default: no explicit output mode set (the node is implicitly projected if matched).
         * - Value: emit the schema node's value.
         * - Shape: emit the schema node's shape.
         * - Decompose: (implies Shape) emit the schema node's leaf values and shape.
         */
        enum class Mode : uint8_t {
            Default = 0,
            Value = 1U << 0,
            Shape = 1U << 1,
            Decompose = 1U << 2,
        };

        // Methods
        [[nodiscard]] auto has(Mode mode) const -> bool {
            if (Mode::Default == mode) {
                return m_mask == 0;
            }
            return (m_mask & static_cast<uint8_t>(mode)) != 0;
        }

        auto merge(NodeMask const& other) -> void { m_mask |= other.m_mask; }

        auto set(Mode mode) -> void {
            m_mask |= static_cast<uint8_t>(mode);
            if (Mode::Decompose == mode) {
                m_mask |= static_cast<uint8_t>(Mode::Shape);
            }
        }

    private:
        // Data members
        uint8_t m_mask{0};
    };

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
     * Adds a column to the set of columns that should be included in the projected results.
     * @param column The column descriptor to project.
     * @param mode The projection output mode.
     * @throws OperationFailed if `column` contains a wildcard
     * @throws OperationFailed if this instance of Projection is in mode ReturnAllColumns
     * @throws OperationFailed if `column` is identical to a previously added column with the same
     * output mode.
     */
    auto add_column(std::shared_ptr<ast::ColumnDescriptor> column, NodeMask::Mode mode) -> void;

    /**
     * Adds a projection column from a FunctionCall (e.g., shape(column) or decompose(column)).
     * @param function_call The function call expression.
     * @throws OperationFailed if the function name is not recognized.
     * @throws OperationFailed if the argument is not a ColumnDescriptor.
     * @throws OperationFailed if the column contains a wildcard.
     * @throws OperationFailed if this instance of Projection is in mode ReturnAllColumns.
     * @throws OperationFailed if the column is identical to a previously added column with the same
     * output type.
     */
    auto add_column(std::shared_ptr<ast::FunctionCall> function_call) -> void;

    /**
     * Checks whether a given schema node has a specific projection mode set.
     * @param node_id The schema node ID to check.
     * @param mode The projection mode to test.
     * @return true if the node has the requested projection mode.
     */
    [[nodiscard]] auto is_projected_as(SchemaNode::id_t node_id, NodeMask::Mode mode) const -> bool;

    /**
     * Gets the projection mask for a given schema node.
     * @param node_id The schema node ID to look up.
     * @return The node's projection mask, or an empty mask if the node has no projections.
     */
    [[nodiscard]] auto get_node_mask(SchemaNode::id_t node_id) const -> NodeMask;

    /**
     * Checks whether a schema node's value should be emitted.
     * @param node_id The schema node ID to check.
     * @return true if the node's value should be emitted.
     */
    [[nodiscard]] auto should_emit_value(SchemaNode::id_t node_id) const -> bool;

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
     * @return true if the column should be included in the output, false if not.
     */
    [[nodiscard]] auto matches_node(SchemaNode::id_t node_id) const -> bool {
        return Mode::ReturnAllColumns == m_projection_mode || m_matching_nodes.contains(node_id);
    }

private:
    // Types
    struct TargetColumn {
        // Constructors
        TargetColumn(
                std::shared_ptr<ast::ColumnDescriptor> column,
                NodeMask::Mode mode,
                std::vector<SchemaNode::id_t> matched_nodes
        )
                : m_column(std::move(column)),
                  m_mode(mode),
                  m_matched_nodes(std::move(matched_nodes)) {}

        // Data members
        std::shared_ptr<ast::ColumnDescriptor> m_column;
        NodeMask::Mode m_mode;
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
     * Records a projection mode for a schema node.
     * @param node_id
     * @param mode
     */
    auto add_projection(SchemaNode::id_t node_id, NodeMask::Mode mode) -> void;

    /**
     * For a resolved column's matched nodes, identifies structural nodes (LogMessage / ParentRule)
     * and records their projection mode. Returns whether any structural node was found.
     * @param tree
     * @param matched_nodes The nodes matched by the column descriptor.
     * @param mode The output mode for this column.
     * @return true if at least one matched node was a structural container.
     */
    [[nodiscard]] auto collect_structural_projections(
            SchemaTree const& tree,
            std::vector<SchemaNode::id_t> const& matched_nodes,
            NodeMask::Mode mode
    ) -> bool;

    // Data members
    std::vector<TargetColumn> m_columns;
    absl::flat_hash_set<SchemaNode::id_t> m_matching_nodes;
    absl::flat_hash_map<SchemaNode::id_t, NodeMask> m_node_projections;
    Mode m_projection_mode{Mode::ReturnAllColumns};
    bool m_allow_duplicate_columns{false};
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_PROJECTION_HPP
