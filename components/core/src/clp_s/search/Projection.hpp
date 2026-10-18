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

    /**
     * How records are projected.
     * - Default: no columns were projected. Every record is emitted in its default form: plain
     *   columns in full, and each StructurizedText as its reconstructed text.
     * - Explicit: only the projected columns are emitted. StructurizedText and ParentRule nodes are
     *   emitted as objects whose fields each come from a projection.
     */
    enum class Mode : uint8_t {
        Default,
        Explicit
    };

    /**
     * The set of output modes active for a MPT node.
     */
    class NodeModes {
    public:
        // Types
        /**
         * Output modes for a projected MPT node.
         * - Default: no explicit output mode set (the node is implicitly projected if matched).
         * - Value: emit the MPT node's value.
         * - Shape: emit the MPT node's shape.
         * - Decompose: (implies Shape) emit the MPT node's leaf values and shape.
         */
        enum class Mode : uint8_t {
            Default = 0,
            Value = 1U << 0,
            Shape = 1U << 1,
            Decompose = 1U << 2,
        };

        // Methods
        /**
         * @param mode
         * @return Whether `mode` is active.
         */
        [[nodiscard]] auto contains(Mode mode) const -> bool {
            if (Mode::Default == mode) {
                return 0 == m_modes;
            }
            return (m_modes & static_cast<uint8_t>(mode)) != 0;
        }

        /**
         * Adds `mode`, keeping any modes already active. Adding `Mode::Decompose` also adds
         * `Mode::Shape`.
         * @param mode
         */
        auto add(Mode mode) -> void {
            m_modes |= static_cast<uint8_t>(mode);
            if (Mode::Decompose == mode) {
                m_modes |= static_cast<uint8_t>(Mode::Shape);
            }
        }

    private:
        // Data members
        uint8_t m_modes{0};
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
     * @throws OperationFailed if `column` contains a wildcard.
     * @throws OperationFailed if this instance of Projection is in mode Default.
     * @throws OperationFailed if `column` is identical to a previously added column.
     */
    auto add_column(std::shared_ptr<ast::ColumnDescriptor> column) -> void;

    /**
     * Checks whether a given schema node has a specific projection mode set.
     * @param node_id The schema node ID to check.
     * @param mode The projection mode to test.
     * @return Whether the node has the requested projection mode.
     */
    [[nodiscard]] auto is_projected_as(mpt_node_id_t node_id, NodeModes::Mode mode) const -> bool;

    /**
     * Gets the projected output modes for a given schema node.
     * @param node_id The schema node ID to look up.
     * @return The node's modes, or no modes if the node has no projections.
     */
    [[nodiscard]] auto get_node_modes(mpt_node_id_t node_id) const -> NodeModes;

    /**
     * Checks whether a schema node's value should be emitted. In the Default mode only
     * StructurizedText nodes emit their value, since their reconstructed text covers every node
     * nested within them. Otherwise a node emits its value when it is projected without `shape()`
     * or `decompose()`.
     * @param node The schema node to check.
     * @return Whether the node's value should be emitted.
     */
    [[nodiscard]] auto should_emit_value(SchemaNode const& node) const -> bool;

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
     * @return Whether the column should be included in the output.
     */
    [[nodiscard]] auto matches_node(mpt_node_id_t node_id) const -> bool {
        return Mode::Default == m_projection_mode || m_matching_nodes.contains(node_id);
    }

    [[nodiscard]] auto get_mode() const -> Mode { return m_projection_mode; }

private:
    // Types
    struct TargetColumn {
        // Constructors
        TargetColumn(std::shared_ptr<ast::ColumnDescriptor> column, NodeModes::Mode mode)
                : m_column(std::move(column)),
                  m_mode(mode) {}

        // Data members
        std::shared_ptr<ast::ColumnDescriptor> m_column;
        NodeModes::Mode m_mode;
        std::vector<mpt_node_id_t> m_matched_nodes;
    };

    // Methods
    /**
     * Resolves an individual column as described by the `resolve_columns` method.
     * @param tree
     * @param column
     * @return The list of schema node IDs matched by the column descriptor.
     */
    [[nodiscard]] auto resolve_column(SchemaTree const& tree, ast::ColumnDescriptor& column)
            -> std::vector<mpt_node_id_t>;

    /**
     * Records a projection mode for a schema node.
     * @param node_id
     * @param mode
     */
    auto add_projection(mpt_node_id_t node_id, NodeModes::Mode mode) -> void;

    /**
     * Records `mode` as the projection mode of every decomposable node among a resolved column's
     * matched nodes.
     * @param tree
     * @param matched_nodes The nodes matched by the column descriptor.
     * @param mode The output mode for this column.
     * @return Whether at least one matched node was decomposable.
     */
    [[nodiscard]] auto collect_decomposable_projections(
            SchemaTree const& tree,
            std::vector<mpt_node_id_t> const& matched_nodes,
            NodeModes::Mode mode
    ) -> bool;

    // Data members
    std::vector<TargetColumn> m_columns;
    absl::flat_hash_set<mpt_node_id_t> m_matching_nodes;
    absl::flat_hash_map<mpt_node_id_t, NodeModes> m_node_projections;
    Mode m_projection_mode{Mode::Default};
    bool m_allow_duplicate_columns{false};
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_PROJECTION_HPP
