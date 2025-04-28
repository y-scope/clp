#ifndef CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLER_HPP
#define CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLER_HPP

#include <outcome/outcome.hpp>

#include "../../KeyValuePairLogEvent.hpp"
#include "../../SchemaTree.hpp"
#include "AstEvaluationResult.hpp"
#include "ErrorCode.hpp"
#include "NewProjectedSchemaTreeNodeCallbackReq.hpp"

namespace clp::ffi::ir_stream::search {
/**
 * Handler for KV-pair IR stream search queries.
 *
 * Each query handler stores a KQL query represented as an AST. The handler is responsible for:
 * - resolving column descriptors for schema-tree nodes within the stream.
 * - evaluating the query against deserialized node-ID-value pairs.
 *
 * @tparam NewProjectedSchemaTreeNodeCallbackType Type of the callback to handle new projected
 * schema-tree nodes.
 */
template <NewProjectedSchemaTreeNodeCallbackReq NewProjectedSchemaTreeNodeCallbackType>
class QueryHandler {
public:
    // Factory function
    /**
     * @param new_projected_schema_tree_node_callback
     * @return A result containing the newly constructed `QueryHandler` on success, or an error code
     * indicating the failure:
     * - TODO
     */
    [[nodiscard]] static auto create(
            NewProjectedSchemaTreeNodeCallbackType new_projected_schema_tree_node_callback
    ) -> outcome_v2::std_result<QueryHandler>;

    // Delete copy constructor and assignment operator
    QueryHandler(QueryHandler const&) = delete;
    auto operator=(QueryHandler const&) -> QueryHandler& = delete;

    // Default move constructor and assignment operator
    QueryHandler(QueryHandler&&) = default;
    auto operator=(QueryHandler&&) -> QueryHandler& = default;

    // Destructor
    ~QueryHandler() = default;

    /**
     * Processes a newly inserted schema-tree node to update any partially-resolved columns.
     * @param is_auto_generated
     * @param node_locator
     * @param node_id
     * @return A void result on success, or an error code indicating the failure:
     * - TODO
     */
    [[nodiscard]] auto update_partially_resolved_columns(
            bool is_auto_generated,
            SchemaTree::NodeLocator const& node_locator,
            SchemaTree::Node::id_t node_id
    ) -> outcome_v2::std_result<void>;

    /**
     * Evaluates the given node-ID-value pairs against the underlying query.
     * @param auto_gen_node_id_value_pairs
     * @param user_gen_node_id_value_pairs
     * @return A result containing the evaluation result on success, or an error code indicating
     * the failure:
     * - TODO
     */
    [[nodiscard]] auto evaluate_node_id_value_pairs(
            KeyValuePairLogEvent::NodeIdValuePairs const& auto_gen_node_id_value_pairs,
            KeyValuePairLogEvent::NodeIdValuePairs const& user_gen_node_id_value_pairs
    ) -> outcome_v2::std_result<AstEvaluationResult>;

private:
    // Constructor
    explicit QueryHandler(
            NewProjectedSchemaTreeNodeCallbackType new_projected_schema_tree_node_callback
    )
            : m_new_projected_schema_tree_node_callback{new_projected_schema_tree_node_callback} {}

    NewProjectedSchemaTreeNodeCallbackType m_new_projected_schema_tree_node_callback;
};

template <NewProjectedSchemaTreeNodeCallbackReq NewProjectedSchemaTreeNodeCallbackType>
auto QueryHandler<NewProjectedSchemaTreeNodeCallbackType>::create(
        [[maybe_unused]] NewProjectedSchemaTreeNodeCallbackType
                new_projected_schema_tree_node_callback
) -> outcome_v2::std_result<QueryHandler<NewProjectedSchemaTreeNodeCallbackType>> {
    return ErrorCode{ErrorCodeEnum::MethodNotImplemented};
}

template <NewProjectedSchemaTreeNodeCallbackReq NewProjectedSchemaTreeNodeCallbackType>
auto QueryHandler<NewProjectedSchemaTreeNodeCallbackType>::update_partially_resolved_columns(
        [[maybe_unused]] bool is_auto_generated,
        [[maybe_unused]] SchemaTree::NodeLocator const& node_locator,
        [[maybe_unused]] SchemaTree::Node::id_t node_id
) -> outcome_v2::std_result<void> {
    return ErrorCode{ErrorCodeEnum::MethodNotImplemented};
}

template <NewProjectedSchemaTreeNodeCallbackReq NewProjectedSchemaTreeNodeCallbackType>
auto QueryHandler<NewProjectedSchemaTreeNodeCallbackType>::evaluate_node_id_value_pairs(
        [[maybe_unused]] KeyValuePairLogEvent::NodeIdValuePairs const& auto_gen_node_id_value_pairs,
        [[maybe_unused]] KeyValuePairLogEvent::NodeIdValuePairs const& user_gen_node_id_value_pairs
) -> outcome_v2::std_result<AstEvaluationResult> {
    return ErrorCode{ErrorCodeEnum::MethodNotImplemented};
}
}  // namespace clp::ffi::ir_stream::search

#endif  // CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLER_HPP
