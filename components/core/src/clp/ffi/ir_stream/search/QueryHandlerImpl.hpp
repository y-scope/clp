#ifndef CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLERIMPL_HPP
#define CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLERIMPL_HPP

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
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
#include "utils.hpp"

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
         * @param column_descriptor
         * @return A result containing the constructed token iterator on success, or an error code
         * indicating the failure:
         * - ErrorCodeEnum::ColumnDescriptorTokenIteratorOutOfBounds if the column doesn't have any
         *   tokens.
         */
        [[nodiscard]] static auto create(clp_s::search::ast::ColumnDescriptor* column_descriptor)
                -> outcome_v2::std_result<ColumnDescriptorTokenIterator> {
            auto const token_begin_it{column_descriptor->descriptor_begin()};
            if (column_descriptor->descriptor_end() == token_begin_it) {
                return ErrorCode{ErrorCodeEnum::ColumnDescriptorTokenIteratorOutOfBounds};
            }
            return ColumnDescriptorTokenIterator{column_descriptor, token_begin_it};
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
         * - ErrorCodeEnum::ColumnDescriptorTokenIteratorOutOfBounds if the current token is already
         *   the last one.
         */
        [[nodiscard]] auto next() const -> outcome_v2::std_result<ColumnDescriptorTokenIterator> {
            if (is_last()) {
                return ErrorCode{ErrorCodeEnum::ColumnDescriptorTokenIteratorOutOfBounds};
            }
            return ColumnDescriptorTokenIterator{m_column_descriptor, m_next_token_it};
        }

        [[nodiscard]] auto is_wildcard() const -> bool { return m_curr_token_it->wildcard(); }

        [[nodiscard]] auto is_trailing_wildcard() const -> bool {
            return is_last() && is_wildcard();
        }

        [[nodiscard]] auto get_token() const -> std::string_view {
            return m_curr_token_it->get_token();
        }

        [[nodiscard]] auto get_column_descriptor() const -> clp_s::search::ast::ColumnDescriptor* {
            return m_column_descriptor;
        }

        /**
         * @param type
         * @return Whether the underlying column can match the given schema-tree node type.
         */
        [[nodiscard]] auto match_schema_tree_node_type(SchemaTree::Node::Type type) const -> bool {
            return m_column_descriptor->matches_any(schema_tree_node_type_to_literal_types(type));
        }

    private:
        // Constructor
        ColumnDescriptorTokenIterator(
                clp_s::search::ast::ColumnDescriptor* column_descriptor,
                clp_s::search::ast::DescriptorList::iterator curr_token_it
        )
                : m_column_descriptor{column_descriptor},
                  m_curr_token_it{curr_token_it},
                  m_next_token_it{curr_token_it + 1} {}

        clp_s::search::ast::ColumnDescriptor* m_column_descriptor;
        clp_s::search::ast::DescriptorList::iterator m_curr_token_it;
        clp_s::search::ast::DescriptorList::iterator m_next_token_it;
    };

    using ProjectionMap = std::unordered_map<clp_s::search::ast::ColumnDescriptor*, std::string>;

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
     * - Forwards `create_projected_columns_and_projection_map`'s return values.
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
     * - Forwards `ColumnDescriptorTokenIterator::next`'s return values.
     * - Forwards `handle_column_resolution_on_new_schema_tree_node`'s return values.
     */
    template <NewProjectedSchemaTreeNodeCallbackReq NewProjectedSchemaTreeNodeCallbackType>
    [[nodiscard]] auto update_partially_resolved_columns(
            bool is_auto_generated,
            SchemaTree::NodeLocator const& node_locator,
            SchemaTree::Node::id_t node_id,
            NewProjectedSchemaTreeNodeCallbackType new_projected_schema_tree_node_callback
    ) -> outcome_v2::std_result<void>;

    [[nodiscard]] auto get_resolved_column_to_schema_tree_node_ids() const -> std::unordered_map<
            clp_s::search::ast::ColumnDescriptor*,
            std::unordered_set<SchemaTree::Node::id_t>> {
        return m_resolved_column_to_schema_tree_node_ids;
    }

private:
    // Constructor
    QueryHandlerImpl(
            std::shared_ptr<clp_s::search::ast::Expression> query,
            PartialResolutionMap auto_gen_namespace_partial_resolutions,
            PartialResolutionMap user_gen_namespace_partial_resolutions,
            std::vector<std::shared_ptr<clp_s::search::ast::ColumnDescriptor>> projected_columns,
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
              m_projected_columns{std::move(projected_columns)},
              m_projected_column_to_original_key{std::move(projected_column_to_original_key)},
              m_case_sensitive_search{case_sensitive_match} {}

    // Methods
    /**
     * Handles column resolution of the given token iterator on the a new schema-tree tree node.
     * @tparam NewProjectedSchemaTreeNodeCallbackType
     * @param is_auto_generated
     * @param node_id
     * @param node_locator
     * @param token_it The current resolved token in the column.
     * @param new_projected_schema_tree_node_callback
     * @return A void result on success, or an error code indicating the failure:
     * - Forwards `ColumnDescriptorTokenIterator::next`'s return values.
     * - Forwards `new_projected_schema_tree_node_callback`'s return values.
     */
    template <NewProjectedSchemaTreeNodeCallbackReq NewProjectedSchemaTreeNodeCallbackType>
    [[nodiscard]] auto handle_column_resolution_on_new_schema_tree_node(
            bool is_auto_generated,
            SchemaTree::Node::id_t node_id,
            SchemaTree::NodeLocator const& node_locator,
            ColumnDescriptorTokenIterator const& token_it,
            NewProjectedSchemaTreeNodeCallbackType new_projected_schema_tree_node_callback
    ) -> outcome_v2::std_result<void>;

    // Variables
    std::shared_ptr<clp_s::search::ast::Expression> m_query;
    PartialResolutionMap m_auto_gen_namespace_partial_resolutions;
    PartialResolutionMap m_user_gen_namespace_partial_resolutions;
    std::unordered_map<
            clp_s::search::ast::ColumnDescriptor*,
            std::unordered_set<SchemaTree::Node::id_t>>
            m_resolved_column_to_schema_tree_node_ids;
    std::vector<std::shared_ptr<clp_s::search::ast::ColumnDescriptor>> m_projected_columns;
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
    auto const parent_node_id{node_locator.get_parent_id()};
    auto& partial_resolutions_to_update{
            is_auto_generated ? m_auto_gen_namespace_partial_resolutions
                              : m_user_gen_namespace_partial_resolutions
    };
    if (false == partial_resolutions_to_update.contains(parent_node_id)) {
        return outcome_v2::success();
    }

    std::vector<std::pair<SchemaTree::Node::id_t, ColumnDescriptorTokenIterator>>
            new_partial_resolutions;
    for (auto const& token_it : partial_resolutions_to_update.at(parent_node_id)) {
        OUTCOME_TRYV(handle_column_resolution_on_new_schema_tree_node(
                is_auto_generated,
                node_id,
                node_locator,
                token_it,
                new_projected_schema_tree_node_callback
        ));

        // Handle partial resolution updates
        if (SchemaTree::Node::Type::Obj != node_locator.get_type()) {
            // If the schema-tree node is not of type `Obj`, it cannot have children, so no further
            // resolution is possible from this node.
            continue;
        }

        if (token_it.is_wildcard()) {
            // For wildcard tokens, simulate potential matches by:
            // - Adding a resolution that assumes this node matches part of the wildcard.
            // - If this isn't the last wildcard token, also add a resolution for continuing the
            //   match.
            new_partial_resolutions.emplace_back(node_id, token_it);
            if (false == token_it.is_last()) {
                new_partial_resolutions.emplace_back(node_id, OUTCOME_TRYX(token_it.next()));
            }
            continue;
        }

        if (token_it.is_last() || token_it.get_token() != node_locator.get_key_name()) {
            // If this is the last token, or the token doesn't match the node's key, then no further
            // resolution is possible through the given node.
            continue;
        }

        auto const next_token_it{OUTCOME_TRYX(token_it.next())};
        new_partial_resolutions.emplace_back(node_id, next_token_it);
        if (false == next_token_it.is_last() && next_token_it.is_wildcard()) {
            // Handle the case where the wildcard matches nothing
            new_partial_resolutions.emplace_back(node_id, OUTCOME_TRYX(next_token_it.next()));
        }
    }

    for (auto const [node_id, token_it] : new_partial_resolutions) {
        auto [it, inserted] = partial_resolutions_to_update.try_emplace(
                node_id,
                std::vector<ColumnDescriptorTokenIterator>{}
        );
        it->second.emplace_back(token_it);
    }

    return outcome_v2::success();
}

template <NewProjectedSchemaTreeNodeCallbackReq NewProjectedSchemaTreeNodeCallbackType>
auto QueryHandlerImpl::handle_column_resolution_on_new_schema_tree_node(
        bool is_auto_generated,
        SchemaTree::Node::id_t node_id,
        SchemaTree::NodeLocator const& node_locator,
        QueryHandlerImpl::ColumnDescriptorTokenIterator const& token_it,
        NewProjectedSchemaTreeNodeCallbackType new_projected_schema_tree_node_callback
) -> outcome_v2::std_result<void> {
    if ((false == token_it.is_last()
         && false == OUTCOME_TRYX(token_it.next()).is_trailing_wildcard())
        || false == token_it.match_schema_tree_node_type(node_locator.get_type())
        || (false == token_it.is_wildcard() && token_it.get_token() != node_locator.get_key_name()))
    {
        // If the given token:
        // - is neither the end token nor the last token before a trailing wildcard
        // - doesn't match the new node's type
        // - is neither a wildcard nor equal to the new node's key
        // There should be no resolution.
        return outcome_v2::success();
    }

    auto* col{token_it.get_column_descriptor()};
    if (m_projected_column_to_original_key.contains(col)) {
        OUTCOME_TRYV(new_projected_schema_tree_node_callback(
                is_auto_generated,
                node_id,
                m_projected_column_to_original_key.at(col)
        ));
        return outcome_v2::success();
    }

    auto [it, inserted] = m_resolved_column_to_schema_tree_node_ids.try_emplace(
            col,
            std::unordered_set<SchemaTree::Node::id_t>{}
    );
    it->second.emplace(node_id);
    return outcome_v2::success();
}
}  // namespace clp::ffi::ir_stream::search

#endif  // CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLERIMPL_HPP
