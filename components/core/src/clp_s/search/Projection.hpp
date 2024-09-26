#ifndef CLP_S_SEARCH_PROJECTION_HPP
#define CLP_S_SEARCH_PROJECTION_HPP

#include <vector>

#include <absl/container/flat_hash_set.h>

#include "../SchemaTree.hpp"
#include "../TraceableException.hpp"
#include "ColumnDescriptor.hpp"

namespace clp_s::search {
enum ProjectionMode : uint8_t {
    ReturnAllColumns,
    ReturnSelectedColumns
};

/**
 * This class describes the set of columns that should be included in the projected results.
 *
 * After adding columns and before calling matches_node the caller is responsible for calling
 * resolve_columns.
 */
class Projection {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    explicit Projection(ProjectionMode mode) : m_projection_mode{mode} {}

    /**
     * Adds a column to the set of columns that should be included in the projected results
     * @param column
     * @throws OperationFailed if `column` contains a wildcard
     * @throws OperationFailed if this instance of Projection is in mode ReturnAllColumns
     * @throws OperationFailed if `column` is identical to a previously added column
     */
    void add_column(std::shared_ptr<ColumnDescriptor> column);

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
    void resolve_columns(std::shared_ptr<SchemaTree> tree);

    /**
     * Checks whether a column corresponding to given leaf node should be included in the output
     * @param node_id
     * @return true if the column should be included in the output, false otherwise
     */
    bool matches_node(int32_t node_id) const {
        return ProjectionMode::ReturnAllColumns == m_projection_mode
               || m_matching_nodes.contains(node_id);
    }

private:
    /**
     * Resolves an individual column as described by the `resolve_columns` method.
     * @param tree
     * @param column
     */
    void resolve_column(std::shared_ptr<SchemaTree> tree, std::shared_ptr<ColumnDescriptor> column);

    std::vector<std::shared_ptr<ColumnDescriptor>> m_selected_columns;
    absl::flat_hash_set<int32_t> m_matching_nodes;
    ProjectionMode m_projection_mode{ProjectionMode::ReturnAllColumns};
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_PROJECTION_HPP
