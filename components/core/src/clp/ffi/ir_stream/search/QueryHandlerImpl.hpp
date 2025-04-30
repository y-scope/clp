#ifndef CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLERIMPL_HPP
#define CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLERIMPL_HPP

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <outcome/outcome.hpp>

#include "../../../../clp_s/search/ast/ColumnDescriptor.hpp"
#include "../../../../clp_s/search/ast/Expression.hpp"
#include "../../../../clp_s/search/ast/Literal.hpp"
#include "../../KeyValuePairLogEvent.hpp"
#include "../../SchemaTree.hpp"
#include "AstEvaluationResult.hpp"
#include "ErrorCode.hpp"
#include "NewProjectedSchemaTreeNodeCallbackReq.hpp"

namespace clp::ffi::ir_stream::search {
/**
 * Implementation of `QueryHandler`.
 */
class QueryHandlerImpl {
public:
    // Types
    /**
     * Iterator of column descriptor tokens.
     */
    class ColumnDescriptorTokenIterator {
    public:
        // Factory function
        /**
         * Creates an iterator that points to the first token in the given column descriptor.
         * @param colum_descriptor
         * @return A result containing the constructed token iterator on success, or an error code
         * indicating the failure:
         * - ErrorCodeEnum::ColumnDescriptorTokenIteratorOutOfBound if the column doesn't have any
         *   token.
         */
        [[nodiscard]] static auto create(clp_s::search::ast::ColumnDescriptor* colum_descriptor)
                -> outcome_v2::std_result<ColumnDescriptorTokenIterator> {
            auto const token_begin_it{colum_descriptor->descriptor_begin()};
            if (colum_descriptor->descriptor_end() == token_begin_it) {
                return ErrorCode{ErrorCodeEnum::ColumnDescriptorTokenIteratorOutOfBound};
            }
            return ColumnDescriptorTokenIterator{colum_descriptor, token_begin_it};
        }

        // Methods
        /**
         * @return Whether this is the last token of the underlying column descriptor.
         */
        [[nodiscard]] auto is_last() const -> bool {
            return m_column_descriptor->descriptor_end() == m_next_token_it;
        }

        /**
         * @return A result containing a newly constructor iterator pointing to the next token on
         * success, or an error code indicating the failure:
         * - ErrorCodeEnum::ColumnDescriptorTokenIteratorOutOfBound if the current token is already
         *   the last one.
         */
        [[nodiscard]] auto next() const -> outcome_v2::std_result<ColumnDescriptorTokenIterator> {
            if (is_last()) {
                return ErrorCode{ErrorCodeEnum::ColumnDescriptorTokenIteratorOutOfBound};
            }
            return ColumnDescriptorTokenIterator{m_column_descriptor, m_next_token_it};
        }

        [[nodiscard]] auto is_wildcard() const -> bool { return m_curr_token_it->wildcard(); }

        [[nodiscard]] auto get_curr_token() const -> std::string_view {
            return m_curr_token_it->get_token();
        }

        [[nodiscard]] auto get_column_descriptor() const -> clp_s::search::ast::ColumnDescriptor* {
            return m_column_descriptor;
        }

        /**
         * @param type
         * @return Whether the underlying column can match the given schema tree node type.
         */
        // TODO: Fix clang-tidy
        // NOLINTNEXTLINE(*)
        [[nodiscard]] auto match(SchemaTree::Node::Type type) const -> bool {
            // TODO: Implement this method after #856 is merged.
            return false;
        }

    private:
        // Constructor
        ColumnDescriptorTokenIterator(
                clp_s::search::ast::ColumnDescriptor* colum_descriptor,
                clp_s::search::ast::DescriptorList::iterator curr_token_it
        )
                : m_column_descriptor{colum_descriptor},
                  m_curr_token_it{curr_token_it},
                  m_next_token_it{curr_token_it + 1} {}

        clp_s::search::ast::ColumnDescriptor* m_column_descriptor;
        clp_s::search::ast::DescriptorList::iterator m_curr_token_it;
        clp_s::search::ast::DescriptorList::iterator m_next_token_it;
    };

    using ProjectionMap = std::
            unordered_map<std::shared_ptr<clp_s::search::ast::ColumnDescriptor>, std::string>;

    using PartialResolutionMap = std::
            unordered_map<SchemaTree::Node::id_t, std::vector<ColumnDescriptorTokenIterator>>;

    // Factory function
    /**
     * @param query The search query.
     * @param projections The columns to project.
     * @param case_sensitive_match Whether to use case-sensitive match for string comparison.
     * @return A result containing the newly constructed `QueryHandler` on success, or an error code
     * indicating the failure:
     * - Forwards `preprocess_query`'s return values.
     * - Forwards `create_projection_map`'s return values.
     * - Forwards `create_initial_partial_resolutions`'s return values.
     */
    [[nodiscard]] static auto create(
            std::shared_ptr<clp_s::search::ast::Expression> query,
            std::vector<std::pair<std::string, clp_s::search::ast::LiteralTypeBitmask>> const&
                    projections,
            bool case_sensitive_match
    ) -> outcome_v2::std_result<QueryHandlerImpl>;

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
    // Constructor
    QueryHandlerImpl(
            std::shared_ptr<clp_s::search::ast::Expression> query,
            PartialResolutionMap auto_gen_namespace_partial_resolutions,
            PartialResolutionMap user_gen_namespace_partial_resolutions,
            ProjectionMap projected_column_to_original_key,
            bool case_sensitive_match
    )
            : m_query{std::move(query)},
              m_auto_gen_namespace_partial_resolutions{
                      std::move(auto_gen_namespace_partial_resolutions)
              },
              m_user_gen_namespace_partial_resolutions{
                      std::move(user_gen_namespace_partial_resolutions)
              },
              m_projected_column_to_original_key{std::move(projected_column_to_original_key)},
              m_case_sensitive_search{case_sensitive_match} {}

    // Variables
    std::shared_ptr<clp_s::search::ast::Expression> m_query;
    PartialResolutionMap m_auto_gen_namespace_partial_resolutions;
    PartialResolutionMap m_user_gen_namespace_partial_resolutions;
    std::unordered_map<clp_s::search::ast::ColumnDescriptor*, std::vector<SchemaTree::Node::id_t>>
            m_resolved_column_to_schema_tree_node_ids;
    ProjectionMap m_projected_column_to_original_key;
    bool m_case_sensitive_search;
};

template <NewProjectedSchemaTreeNodeCallbackReq NewProjectedSchemaTreeNodeCallbackType>
auto QueryHandlerImpl::update_partially_resolved_columns(
        bool is_auto_generated,
        SchemaTree::NodeLocator const& node_locator,
        SchemaTree::Node::id_t node_id,
        NewProjectedSchemaTreeNodeCallbackType new_projected_schema_tree_node_callback
) -> outcome_v2::std_result<void> {
    return ErrorCode{ErrorCodeEnum::MethodNotImplemented};
}
}  // namespace clp::ffi::ir_stream::search

#endif  // CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLERIMPL_HPP
