#ifndef CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLERIMPL_HPP
#define CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLERIMPL_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <outcome/outcome.hpp>

#include "../../../../clp_s/search/ast/ColumnDescriptor.hpp"
#include "../../../../clp_s/search/ast/Expression.hpp"
#include "../../KeyValuePairLogEvent.hpp"
#include "../../SchemaTree.hpp"
#include "AstEvaluationResult.hpp"
#include "NewProjectedSchemaTreeNodeCallbackReq.hpp"

namespace clp::ffi::ir_stream::search {
/**
 * Implementation of `QueryHandler`.
 */
class QueryHandlerImpl {
public:
    // Factory function
    /**
     * @return A result containing the newly constructed `QueryHandler` on success, or an error code
     * indicating the failure:
     * - TODO
     */
    [[nodiscard]] static auto create() -> outcome_v2::std_result<QueryHandlerImpl>;

    // Delete copy constructor and assignment operator
    QueryHandlerImpl(QueryHandlerImpl const&) = delete;
    auto operator=(QueryHandlerImpl const&) -> QueryHandlerImpl& = delete;

    // Default move constructor and assignment operator
    QueryHandlerImpl(QueryHandlerImpl&&) = default;
    auto operator=(QueryHandlerImpl&&) -> QueryHandlerImpl& = default;

    // Destructor
    ~QueryHandlerImpl() = default;

    /**
     * Implementation of `QueryHandler::evaluate_node_id_value_pairs`.
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

    /**
     * Implementation of `QueryHandler::update_partially_resolved_columns` with new projected
     * schema-tree node callback given as a template parameter.
     * @tparam NewProjectedSchemaTreeNodeCallbackType
     * @param auto_gen_node_id_value_pairs
     * @param user_gen_node_id_value_pairs
     * @param new_projected_schema_tree_node_callback
     * @return A result containing the evaluation result on success, or an error code indicating
     * the failure:
     * - TODO
     */
    template <NewProjectedSchemaTreeNodeCallbackReq NewProjectedSchemaTreeNodeCallbackType>
    [[nodiscard]] auto update_partially_resolved_columns(
            bool is_auto_generated,
            SchemaTree::NodeLocator const& node_locator,
            SchemaTree::Node::id_t node_id,
            NewProjectedSchemaTreeNodeCallbackType new_projected_schema_tree_node_callback
    ) -> outcome_v2::std_result<void>;

private:
    // Types
    using PairNamePlaceholder = std::pair<
            clp_s::search::ast::ColumnDescriptor*,
            clp_s::search::ast::DescriptorList::iterator>;

    // Constructor

    // Variables
    std::shared_ptr<clp_s::search::ast::Expression> m_query;
    std::unordered_map<SchemaTree::Node::id_t, std::vector<PairNamePlaceholder>>
            m_auto_gen_namespace_partial_resolutions;
    std::unordered_map<SchemaTree::Node::id_t, std::vector<PairNamePlaceholder>>
            m_user_gen_namespace_partial_resolutions;
    std::unordered_map<clp_s::search::ast::ColumnDescriptor*, SchemaTree::Node::id_t>
            m_resolved_column_to_schema_tree_node_ids;
    std::unordered_map<clp_s::search::ast::ColumnDescriptor*, std::string>
            m_projected_column_to_original_key;
    bool m_case_sensitive_search;
};
}  // namespace clp::ffi::ir_stream::search

#endif  // CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLERIMPL_HPP
