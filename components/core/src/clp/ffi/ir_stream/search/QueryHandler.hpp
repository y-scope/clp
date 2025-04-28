#ifndef CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLER_HPP
#define CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLER_HPP

#include <outcome/outcome.hpp>

#include "../../SchemaTree.hpp"
#include "NewProjectedSchemaTreeNodeCallbackReq.hpp"

namespace clp::ffi::ir_stream::search {
template <NewProjectedSchemaTreeNodeCallbackReq NewProjectedSchemaTreeNodeCallbackType>
class QueryHandler {
public:
    // Delete copy constructor and assignment operator
    QueryHandler(QueryHandler const&) = delete;
    auto operator=(QueryHandler const&) -> QueryHandler& = delete;

    // Default move constructor and assignment operator
    QueryHandler(QueryHandler&&) = default;
    auto operator=(QueryHandler&&) -> QueryHandler& = default;

    // Destructor
    ~QueryHandler() = default;

    /**
     * Handles a schema tree node insertion to update the underlying column resolution.
     * @param is_auto_generated
     * @param node_locator
     * @param node_id
     * @return A void result on success, or an error code indicating the failure:
     * - TODO
     */
    [[nodiscard]] auto handle_schema_tree_node_insertion(
            bool is_auto_generated,
            SchemaTree::NodeLocator const& node_locator,
            SchemaTree::Node::id_t node_id
    ) -> outcome_v2::std_result<void>;

    [[nodiscard]] auto evaluate_node_id_value_pairs() -> outcome_v2::std_result<void>;

private:
    // Constructor
    QueryHandler(NewProjectedSchemaTreeNodeCallbackType new_projected_schema_tree_node_callback)
            : m_new_projected_schema_tree_node_callback{new_projected_schema_tree_node_callback} {}

    NewProjectedSchemaTreeNodeCallbackType m_new_projected_schema_tree_node_callback;
};
}  // namespace clp::ffi::ir_stream::search

#endif  // CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLER_HPP
