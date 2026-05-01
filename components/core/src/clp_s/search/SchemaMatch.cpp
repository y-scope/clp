#include "SchemaMatch.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <log_surgeon/log_surgeon.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/search/ast/StringLiteral.hpp>
#include <clpp/DecomposedQuery.hpp>
#include <clpp/ErrorCode.hpp>

#include "../archive_constants.hpp"
#include "../SchemaTree.hpp"
#include "ast/AndExpr.hpp"
#include "ast/ColumnDescriptor.hpp"
#include "ast/ConstantProp.hpp"
#include "ast/EmptyExpr.hpp"
#include "ast/Expression.hpp"
#include "ast/FilterExpr.hpp"
#include "ast/FilterOperation.hpp"
#include "ast/Literal.hpp"
#include "ast/OrExpr.hpp"
#include "ast/OrOfAndForm.hpp"
#include "clp_s/ArchiveReader.hpp"
#include "clp_s/Defs.hpp"

using clp_s::search::ast::AndExpr;
using clp_s::search::ast::ColumnDescriptor;
using clp_s::search::ast::ConstantProp;
using clp_s::search::ast::DescriptorList;
using clp_s::search::ast::DescriptorToken;
using clp_s::search::ast::EmptyExpr;
using clp_s::search::ast::Expression;
using clp_s::search::ast::FilterExpr;
using clp_s::search::ast::FilterOperation;
using clp_s::search::ast::literal_type_bitmask_t;
using clp_s::search::ast::LiteralType;
using clp_s::search::ast::OrExpr;
using clp_s::search::ast::OrOfAndForm;

namespace clp_s::search {
namespace {
auto build_leaf_query_expr(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        clpp::DecomposedQuery const& decomposed_query
) -> std::shared_ptr<ast::Expression>;

/**
 * Gets the `NodeType` corresponding to a given subtree type.
 * @param subtree_type
 * @return the corresponding `NodeType` or `NodeType::Unknown` if the subtree type is unknown.
 */
auto get_subtree_node_type(std::string_view subtree_type) -> NodeType;

auto build_leaf_query_expr(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        clpp::DecomposedQuery const& decomposed_query
) -> std::shared_ptr<ast::Expression> {
    auto leaves_expr{ast::AndExpr::create()};
    for (auto const& leaf : decomposed_query.get_leaf_queries()) {
        auto new_col{column->copy()};
        new_col->set_matching_types(
                LiteralType::FloatT | LiteralType::IntegerT | LiteralType::VarStringT
        );
        auto& new_col_descriptors{new_col->get_descriptor_list()};

        auto start{leaf.m_names.rbegin()};
        if (new_col_descriptors.back().get_token() == *start) {
            ++start;
        }
        for (auto it{start}; leaf.m_names.rend() != it; ++it) {
            new_col_descriptors.emplace_back(
                    DescriptorToken::create_descriptor_from_literal_token(*it)
            );
        }
        auto leaf_literal{ast::StringLiteral::create(leaf.m_query)};
        leaves_expr->add_operand(
                FilterExpr::create(new_col, ast::FilterOperation::EQ, leaf_literal)
        );
    }
    return leaves_expr;
}

auto get_subtree_node_type(std::string_view subtree_type) -> NodeType {
    if (constants::cMetadataSubtreeType == subtree_type) {
        return NodeType::Metadata;
    }
    if (constants::cObjectSubtreeType == subtree_type) {
        return NodeType::Object;
    }
    return NodeType::Unknown;
}

auto collect_columns(std::shared_ptr<Expression> const& cur, std::set<ColumnDescriptor*>& columns)
        -> void {
    for (auto it{cur->op_begin()}; it != cur->op_end(); ++it) {
        if (auto sub_expr{std::dynamic_pointer_cast<Expression>(*it)}; nullptr != sub_expr) {
            collect_columns(sub_expr, columns);
        } else if (auto column{std::dynamic_pointer_cast<ColumnDescriptor>(*it)}; nullptr != column)
        {
            columns.insert(column.get());
        }
    }
}

auto convert_lt_ids_to_schema_ids(
        std::vector<clp_s::logtype_id_t> const& matched_lt_ids,
        std::unordered_map<clp_s::logtype_id_t, std::vector<int32_t>> const& lt_to_schema_map
) -> std::unordered_set<int32_t> {
    std::unordered_set<int32_t> schema_ids;
    for (auto const id : matched_lt_ids) {
        if (auto const it{lt_to_schema_map.find(id)}; lt_to_schema_map.end() != it) {
            schema_ids.insert(it->second.begin(), it->second.end());
        }
    }
    return schema_ids;
}
}  // namespace

// TODO: write proper iterators on the AST to make this code less awful.
// In particular schema intersection needs AST iterators and a proper refactor
SchemaMatch::SchemaMatch(std::shared_ptr<ArchiveReader> archive_reader)
        : m_tree(archive_reader->get_schema_tree()),
          m_schemas(archive_reader->get_schema_map()),
          m_archive_reader(std::move(archive_reader)) {}

std::shared_ptr<Expression> SchemaMatch::run(std::shared_ptr<Expression>& expr) {
    build_logtype_id_to_schema_id_map();
    ConstantProp propagate_empty;
    expr = populate_column_mapping(expr);
    expr = propagate_empty.run(expr);
    if (std::dynamic_pointer_cast<EmptyExpr>(expr)) {
        return expr;
    }

    // if we had ambiguous column descriptors containing regex which were
    // resolved we need to restandardize the expression
    if (false == m_unresolved_descriptor_to_descriptor.empty() || m_clpp_decomposed_query) {
        m_column_to_descriptor.clear();
        m_unresolved_descriptor_to_descriptor.clear();

        // restandardize the form, and rerun column mapping
        OrOfAndForm standard_form;
        expr = standard_form.run(expr);
        expr = populate_column_mapping(expr);
    }

    populate_schema_mapping();

    expr = intersect_schemas(expr);
    expr = propagate_empty.run(expr);

    if (std::dynamic_pointer_cast<EmptyExpr>(expr)) {
        return expr;
    }

    split_expression_by_schema(expr, m_schema_to_query, m_matched_schema_ids);

    return expr;
}

std::shared_ptr<Expression> SchemaMatch::populate_column_mapping(
        std::shared_ptr<Expression> const& cur
) {
    for (auto it = cur->op_begin(); it != cur->op_end(); it++) {
        if (auto child = std::dynamic_pointer_cast<Expression>(*it)) {
            auto new_child = populate_column_mapping(child);
            if (new_child != child) {
                new_child->copy_replace(cur.get(), it);
            }
        } else if (auto const column{std::dynamic_pointer_cast<ast::ColumnDescriptor>(*it)};
                   nullptr != column)
        {
            auto [mapped_succesfully, new_and_expr]{populate_column_mapping(column, cur)};
            if (false == mapped_succesfully) {
                // no matching columns -- replace this expression with empty;
                return EmptyExpr::create();
            }

            if (nullptr != new_and_expr) {
                return new_and_expr;
            }

            if (column->is_unresolved_descriptor() && false == column->is_pure_wildcard()) {
                auto possibilities = OrExpr::create();

                // TODO: will have to decide how we wan't to handle multi-column expressions
                // with unresolved descriptors
                for (auto const node_id : m_unresolved_descriptor_to_descriptor.at(column.get())) {
                    auto const* node = &m_tree->get_node(node_id);
                    auto literal_type = node_to_literal_type(node->get_type());
                    DescriptorList descriptors;
                    // FIXME: this needs to be adjusted to handle more than JUST object subtrees
                    // TODO: consider whether fully resolving descriptors in this way is actually
                    // necessary. In principal the set of matching nodes is all that is really
                    // required (and has already been determined) so the main utility of the
                    // following code is for debugging and simply adds overhead in non-debugging
                    // execution. It should be possible to both get rid of this code (only using it
                    // for debugging) and change how this pass works to only run column resolution a
                    // single time. Specifically there doesn't seem to be anything stopping us from
                    // just doing `resolved_column->set_column_id(node_id)` and skipping populating
                    // the descriptors/re-resolving the columns after normalization. Actually in
                    // some contrived circumstances involving objects in arrays while the array
                    // structurization feature is enabled it seems like the current flow where we
                    // re-run column resolution after this can make these columns again match
                    // multiple nodes.
                    while (node->get_id()
                           != m_tree->get_object_subtree_node_id_for_namespace(
                                   column->get_namespace()
                           ))
                    {
                        descriptors.emplace_back(
                                DescriptorToken::create_descriptor_from_literal_token(
                                        node->get_key_name()
                                )
                        );
                        node = &m_tree->get_node(node->get_parent_id());
                    }
                    std::reverse(descriptors.begin(), descriptors.end());
                    auto resolved_column = ColumnDescriptor::create_from_descriptors(
                            descriptors,
                            column->get_namespace()
                    );
                    resolved_column->set_matching_type(literal_type);
                    *it = resolved_column;
                    cur->copy_append(possibilities.get());
                }
                return possibilities;
            }
        }
    }
    return cur;
}

auto SchemaMatch::populate_column_mapping(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        std::shared_ptr<ast::Expression> const& expr
) -> std::tuple<bool, std::shared_ptr<ast::Expression>> {
    bool matched = false;
    // TODO: consider making this loop (and dynamic wildcard expansion in general) respect
    // namespaces.
    // TODO: consider removing this imprecise loop when we resolve issue #907.
    if (column->is_pure_wildcard()) {
        for (auto const& node : m_tree->get_nodes()) {
            if (column->matches_type(node_to_literal_type(node.get_type()))) {
                // column_to_descriptor_[node->get_id()].insert(column);
                //  At least some node matches; break
                //  Don't use column_to_descriptor_ for pure wildcard columns anyway, so
                //  no need to waste memory
                matched = true;
                break;
            }
        }

        return {matched, expr};
    }

    auto resolve_against_subtree =
            [&](SchemaNode const& root_node) -> std::tuple<bool, std::shared_ptr<ast::Expression>> {
        for (auto const child_node_id : root_node.get_children_ids()) {
            // TODO clpp: ???
            auto [matched, new_expr]{populate_column_mapping(column, child_node_id, expr)};
            if (matched) {
                return {matched, new_expr};
            }
        }
        return {matched, expr};
    };

    if (auto const& subtree_type{column->get_subtree_type()}; subtree_type.has_value()) {
        // Resolve against the subtree with matching namespace and type if it exists.
        auto const node_type{get_subtree_node_type(subtree_type.value())};
        if (auto const subtree_root_node_id
            = m_tree->get_subtree_node_id(column->get_namespace(), node_type);
            -1 != subtree_root_node_id)
        {
            auto const& root_node = m_tree->get_node(subtree_root_node_id);
            // TODO clpp: ???
            return resolve_against_subtree(root_node);
        }
    } else {
        // Resolve against every subtree that has matching namespaces except for the
        // `NodeType::Metadata` subtree.
        for (auto const& [namespace_type_pair, subtree_root_node_id] : m_tree->get_subtrees()) {
            if (NodeType::Metadata != namespace_type_pair.second
                && namespace_type_pair.first == column->get_namespace())
            {
                auto const& root_node = m_tree->get_node(subtree_root_node_id);
                // TODO clpp: ???
                return resolve_against_subtree(root_node);
            }
        }
    }
    return {matched, expr};
}

auto SchemaMatch::populate_column_mapping(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        int32_t node_id,
        std::shared_ptr<ast::Expression> const& expr
) -> std::tuple<bool, std::shared_ptr<ast::Expression>> {
    /**
     * This function is the core of Column Resolution. The general idea is to walk down different
     * branches of the mst while advancing an iterator over the column descriptor in step.
     *
     * There are a few notable edge cases we handle here namely
     * 	1) wildcard tokens must be allowed to match any number of mst nodes including zero
     * 	2) mst node entries with no name are automatically accepted, do not advance the token
     * 	   iterator, and can be accepted in recursive descent even if the token iterator is at the
     * 	   end
     */
    using state = std::tuple<int32_t, DescriptorList::iterator, int32_t>;
    std::priority_queue<state, std::vector<state>, std::greater<state>> work_list;
    std::set<std::pair<DescriptorList::iterator, int32_t>> visited_states;
    auto it_start = column->descriptor_begin();
    work_list.emplace(std::make_tuple(0, it_start, node_id));
    // Allow matching a wildcard zero times
    if (column->descriptor_end() != it_start && it_start->wildcard()) {
        work_list.emplace(std::make_tuple(0, ++it_start, node_id));
    }
    int32_t prev_level = 0;
    bool matched = false;
    while (false == work_list.empty()) {
        auto& cur = work_list.top();
        auto [cur_depth, cur_it, cur_node_id] = cur;
        work_list.pop();
        if (prev_level != cur_depth) {
            prev_level = cur_depth;
            visited_states.clear();
        }

        // Make sure we haven't visited this state yet via different routes of resolving wildcards
        auto cur_state = std::make_pair(cur_it, cur_node_id);
        if (visited_states.count(cur_state) > 0) {
            continue;
        }
        visited_states.emplace(cur_state);

        // Check if the current node is accepted
        auto const& cur_node = m_tree->get_node(cur_node_id);
        bool is_key_name_empty = cur_node.get_key_name().empty();
        bool at_descriptor_list_end = cur_it == column->descriptor_end();
        auto next_it = cur_it;
        if (false == at_descriptor_list_end) {
            ++next_it;
        }
        bool next_at_descriptor_list_end = next_it == column->descriptor_end();
        bool wildcard_descriptor = false == at_descriptor_list_end && cur_it->wildcard();
        bool accepted = false;

        if (wildcard_descriptor) {
            accepted = true;
        } else if (is_key_name_empty) {
            accepted = true;
        } else if ((false == at_descriptor_list_end
                    && cur_node.get_key_name() == cur_it->get_token()))
        {
            accepted = true;
        }

        // Check if the current node is matched
        if (false == accepted) {
            continue;
        }

        // Currently we only allow fully resolved descriptors for precise array search
        if (NodeType::UnstructuredArray == cur_node.get_type()
            && false == column->is_unresolved_descriptor())
        {
            /**
             * TODO: This doesn't work in general, but it had the same limitation in the previous
             * implementation, so I will leave it broken for now.
             *
             * E.g. breaks for a query like `a.b.c:d` on the collection of objects
             * {"a": [{"b": {"c": "d"}}]}
             * {"a": {"b": [{"c": "d"}]}}
             */
            column->add_unresolved_tokens(next_it);
            auto [descriptors_it, _] = m_column_to_descriptor.try_emplace(cur_node_id);
            descriptors_it->second.emplace(column);
            matched = true;
            continue;
        } else if ((next_at_descriptor_list_end
                    && column->matches_type(node_to_literal_type(cur_node.get_type()))))
        {
            if (false == column->is_unresolved_descriptor()) {
                if (NodeType::LogMessage == cur_node.get_type()
                    || NodeType::ParentRule == cur_node.get_type())
                {
                    m_clpp_decomposed_query = true;

                    auto* filter{dynamic_cast<FilterExpr*>(expr.get())};
                    std::string query;
                    filter->get_operand()->as_clp_string(query, filter->get_operation());

                    auto& typed_dict{*m_archive_reader->get_typed_log_type_dictionary()};
                    if (typed_dict.get_entries().empty()) {
                        typed_dict.read_entries();
                    }

                    auto [decomposed_query, matched_lt_ids]
                            = (NodeType::LogMessage == cur_node.get_type())
                                      ? decompose_and_match_log_message(query)
                                      : decompose_and_match_parent_var(cur_node, cur_it, query);

                    if (auto result{resolve_clpp_match(
                                column,
                                cur_node_id,
                                decomposed_query,
                                matched_lt_ids
                        )};
                        result.has_value())
                    {
                        return std::move(result).value();
                    }
                    continue;
                }
                auto [descriptors_it, _] = m_column_to_descriptor.try_emplace(cur_node_id);
                descriptors_it->second.emplace(column);
            } else {
                auto [node_ids_it, _]
                        = m_unresolved_descriptor_to_descriptor.try_emplace(column.get());
                node_ids_it->second.emplace(cur_node_id);
            }
            matched = true;
            continue;
        }

        // Allow matching a wildcard zero times
        if (false == next_at_descriptor_list_end && next_it->wildcard()) {
            work_list.emplace(std::make_tuple(cur_depth, next_it, cur_node_id));
        }

        // Push nodes to the work list
        for (int32_t child_node_id : cur_node.get_children_ids()) {
            if (is_key_name_empty) {
                // Don't advance the iterator when accepting an empty key
                work_list.emplace(std::make_tuple(cur_depth + 1, cur_it, child_node_id));
            } else {
                work_list.emplace(std::make_tuple(cur_depth + 1, next_it, child_node_id));
                if (wildcard_descriptor) {
                    // Allow matching a wildcard token multiple times
                    work_list.emplace(std::make_tuple(cur_depth + 1, cur_it, child_node_id));
                }
            }
        }
    }
    return {matched, expr};
}

void SchemaMatch::populate_schema_mapping() {
    // TODO: consider refactoring this to take advantage of the ordered region of the schema
    for (auto& it : *m_schemas) {
        int32_t schema_id = it.first;
        for (int32_t column_id : it.second) {
            if (Schema::schema_entry_is_unordered_object(column_id)) {
                continue;
            }
            if (NodeType::UnstructuredArray == m_tree->get_node(column_id).get_type()) {
                m_array_schema_ids.insert(schema_id);
            }
            if (false == m_column_to_descriptor.contains(column_id)) {
                continue;
            }
            for (auto const& descriptor : m_column_to_descriptor.at(column_id)) {
                if (descriptor->is_pure_wildcard()) {
                    continue;
                }

                auto [schema_to_column_id_it, _]
                        = m_descriptor_to_schema.try_emplace(descriptor.get());
                schema_to_column_id_it->second.emplace(schema_id, column_id);
            }
        }
    }
}

std::shared_ptr<Expression> SchemaMatch::intersect_schemas(std::shared_ptr<Expression> cur) {
    if (std::dynamic_pointer_cast<AndExpr>(cur) || std::dynamic_pointer_cast<FilterExpr>(cur)) {
        std::set<int32_t> common_schema;
        std::set<ColumnDescriptor*> columns;
        intersect_and_sub_expr(cur, common_schema, columns, true);

        if (common_schema.empty()) {
            return EmptyExpr::create(cur->get_parent());
        }

        for (int32_t schema_id : common_schema) {
            m_expression_to_schemas[cur.get()].insert(schema_id);
        }

        for (auto column : columns) {
            if (column->is_pure_wildcard()) {
                continue;
            }

            literal_type_bitmask_t types = 0;
            for (int32_t schema : common_schema) {
                if (m_descriptor_to_schema[column].contains(schema)) {
                    types |= node_to_literal_type(
                            m_tree->get_node(m_descriptor_to_schema[column][schema]).get_type()
                    );
                }
            }
            column->set_matching_types(types);
        }

        for (int32_t schema : common_schema) {
            m_matched_schema_ids.insert(schema);

            for (auto column : columns) {
                if (false == column->is_pure_wildcard()) {
                    m_schema_to_searched_columns[schema].insert(
                            get_column_id_for_descriptor(column, schema)
                    );
                }
            }
        }
    } else if (cur->has_only_expression_operands()) {
        for (auto it = cur->op_begin(); it != cur->op_end(); it++) {
            auto sub_expr = std::static_pointer_cast<Expression>(*it);
            auto new_expr = intersect_schemas(sub_expr);

            if (new_expr != sub_expr) {
                *it = new_expr;
            }
        }
    }
    return cur;
}

auto SchemaMatch::intersect_and_sub_expr(
        std::shared_ptr<Expression> const& cur,
        std::set<int32_t>& common_schema,
        std::set<ColumnDescriptor*>& columns,
        bool first
) -> bool {
    // Note: EmptyExpr are already constant propogated out of the ands, so don't
    // need to check for them here
    for (auto it = cur->op_begin(); it != cur->op_end(); it++) {
        if (auto sub_expr = std::dynamic_pointer_cast<Expression>(*it)) {
            first &= intersect_and_sub_expr(sub_expr, common_schema, columns, first);
            if (false == first && common_schema.empty()) {
                break;
            }
        } else if (auto column = std::dynamic_pointer_cast<ColumnDescriptor>(*it)) {
            FilterOperation op = std::static_pointer_cast<FilterExpr>(cur)->get_operation();
            if ((op != FilterOperation::EXISTS && op != FilterOperation::NEXISTS)
                || column->has_unresolved_tokens())
            {
                columns.insert(column.get());
            }

            if (column->is_pure_wildcard()) {
                // TODO: consider handling `*:null` NEXISTS edgecase here instead of during
                // output
                if (first) {
                    for (auto schema_it : *m_schemas) {
                        common_schema.insert(schema_it.first);
                    }
                }
                return false;
            } else if (first && op != FilterOperation::NEXISTS) {
                for (auto schema_it : m_descriptor_to_schema[column.get()]) {
                    common_schema.insert(schema_it.first);
                }
                return false;
            } else if (first /*&& op == FilterOperation::NEXISTS */) {
                for (auto& schema : *m_schemas) {
                    if (0 == m_descriptor_to_schema[column.get()].count(schema.first)) {
                        common_schema.insert(schema.first);
                    }
                }
                return false;
            } else if (op == FilterOperation::NEXISTS) {
                std::set<int32_t> intersection;
                auto const& cur_schemas = m_descriptor_to_schema[column.get()];
                for (int32_t schema : common_schema) {
                    if (0 == cur_schemas.count(schema)) {
                        intersection.insert(schema);
                    }
                }
                common_schema = intersection;
            } else {
                std::set<int32_t> intersection;
                auto const& cur_schemas = m_descriptor_to_schema[column.get()];
                for (int32_t schema : common_schema) {
                    if (cur_schemas.count(schema)) {
                        intersection.insert(schema);
                    }
                }
                common_schema = intersection;
            }
        }
    }
    return first;
}

void SchemaMatch::split_expression_by_schema(
        std::shared_ptr<Expression> const& expr,
        std::map<int32_t, std::shared_ptr<Expression>>& queries,
        std::unordered_set<int32_t> const& relevant_schemas
) {
    if (auto filter = std::dynamic_pointer_cast<FilterExpr>(expr)) {
        for (int32_t schema_id : relevant_schemas) {
            auto new_filter = filter->copy();
            auto descriptor = std::static_pointer_cast<FilterExpr>(new_filter)->get_column().get();
            auto old_descriptor = filter->get_column().get();

            if (false == descriptor->is_pure_wildcard()) {
                descriptor->set_column_id(get_column_id_for_descriptor(old_descriptor, schema_id));
                auto literal_type = get_literal_type_for_column(old_descriptor, schema_id);
                if (literal_type == LiteralType::ArrayT) {
                    m_array_search_schema_ids.insert(schema_id);
                }
                descriptor->set_matching_type(literal_type);
            } else if ((descriptor->is_pure_wildcard()
                        && descriptor->matches_type(LiteralType::ArrayT)
                        && 0 == m_array_search_schema_ids.count(schema_id)))
            {
                for (auto column_id : (*m_schemas)[schema_id]) {
                    if (Schema::schema_entry_is_unordered_object(column_id)) {
                        continue;
                    }
                    if (NodeType::UnstructuredArray == m_tree->get_node(column_id).get_type()) {
                        m_array_search_schema_ids.insert(schema_id);
                        break;
                    }
                }
            }
            queries[schema_id] = new_filter;
        }
    } else if (std::dynamic_pointer_cast<AndExpr>(expr)) {
        std::map<int32_t, std::shared_ptr<Expression>> sub_expressions;
        for (auto const& op : expr->get_op_list()) {
            auto sub_expr = std::static_pointer_cast<Expression>(op);
            split_expression_by_schema(sub_expr, sub_expressions, relevant_schemas);

            for (auto const& it : sub_expressions) {
                if (queries.count(it.first)) {
                    it.second->copy_append(queries[it.first].get());
                } else {
                    auto parent_expr = AndExpr::create(expr->is_inverted());
                    it.second->copy_append(parent_expr.get());
                    queries[it.first] = parent_expr;
                }
            }

            sub_expressions.clear();
        }
    } else if (std::dynamic_pointer_cast<OrExpr>(expr)) {
        std::map<int32_t, std::shared_ptr<Expression>> sub_expressions;
        for (auto const& op : expr->get_op_list()) {
            auto sub_expr = std::static_pointer_cast<Expression>(op);
            split_expression_by_schema(
                    sub_expr,
                    sub_expressions,
                    m_expression_to_schemas.at(sub_expr.get())
            );

            for (auto const& it : sub_expressions) {
                if (queries.count(it.first)) {
                    auto& cur_subexpr = queries[it.first];
                    if (std::dynamic_pointer_cast<OrExpr>(cur_subexpr)) {
                        it.second->copy_append(cur_subexpr.get());
                    } else {
                        auto parent_expr = OrExpr::create();
                        cur_subexpr->copy_append(parent_expr.get());
                        it.second->copy_append(parent_expr.get());
                        queries[it.first] = parent_expr;
                    }
                } else {
                    queries[it.first] = it.second;
                }
            }

            sub_expressions.clear();
        }

        if (expr->is_inverted()) {
            for (auto const& it : queries) {
                it.second->invert();
            }
        }
    }
}

int32_t SchemaMatch::get_column_id_for_descriptor(ColumnDescriptor* column, int32_t schema) {
    return m_descriptor_to_schema[column][schema];
}

bool SchemaMatch::schema_matched(int32_t schema_id) {
    return m_matched_schema_ids.count(schema_id);
}

bool SchemaMatch::schema_searches_against_column(int32_t schema, int32_t column_id) {
    return m_schema_to_searched_columns[schema].count(column_id);
}

void SchemaMatch::add_searched_column_to_schema(int32_t schema, int32_t column) {
    m_schema_to_searched_columns[schema].insert(column);
}

bool SchemaMatch::has_array(int32_t schema_id) {
    return m_array_schema_ids.count(schema_id);
}

bool SchemaMatch::has_array_search(int32_t schema_id) {
    return m_array_search_schema_ids.count(schema_id);
}

LiteralType SchemaMatch::get_literal_type_for_column(ColumnDescriptor* column, int32_t schema) {
    return node_to_literal_type(
            m_tree->get_node(get_column_id_for_descriptor(column, schema)).get_type()
    );
}

std::shared_ptr<Expression> SchemaMatch::get_query_for_schema(int32_t schema) {
    return m_schema_to_query.at(schema);
}

void SchemaMatch::build_logtype_id_to_schema_id_map() {
    for (auto const& [schema_id, schema] : *m_schemas) {
        for (auto const node_id : schema) {
            if (Schema::schema_entry_is_unordered_object(node_id)) {
                continue;
            }
            if (NodeType::LogTypeID == m_tree->get_node(node_id).get_type()) {
                auto logtype_id
                        = std::stoull(std::string{m_tree->get_node(node_id).get_key_name()});
                m_logtype_id_to_schema_id[static_cast<logtype_id_t>(logtype_id)].emplace_back(
                        schema_id
                );
                break;
            }
        }
    }
}

auto SchemaMatch::resolve_clpp_match(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        int32_t cur_node_id,
        clpp::DecomposedQuery const* decomposed_query,
        std::vector<clp_s::logtype_id_t> const& matched_lt_ids
) -> std::optional<std::tuple<bool, std::shared_ptr<ast::Expression>>> {
    auto matched_schema_ids{
            convert_lt_ids_to_schema_ids(matched_lt_ids, m_logtype_id_to_schema_id)
    };
    if (matched_schema_ids.empty()) {
        return std::nullopt;
    }

    auto leaves_expr{build_leaf_query_expr(column, *decomposed_query)};
    auto const& leaf_queries{decomposed_query->get_leaf_queries()};
    auto op_it{leaves_expr->op_begin()};
    for (size_t leaf_idx{0}; leaf_idx < leaf_queries.size(); ++leaf_idx, ++op_it) {
        auto filter_expr{std::static_pointer_cast<ast::FilterExpr>(*op_it)};
        auto leaf_col{filter_expr->get_column()};

        int32_t node_id{cur_node_id};
        auto const& leaf{leaf_queries[leaf_idx]};
        auto start{leaf.m_names.rbegin()};
        if (false == leaf.m_names.empty()) {
            if (auto const& column_descriptors{column->get_descriptor_list()};
                false == column_descriptors.empty()
                && column_descriptors.back().get_token() == *start)
            {
                ++start;
            }
        }
        for (auto it{start}; leaf.m_names.rend() != it; ++it) {
            bool found{false};
            for (auto child_id : m_tree->get_node(node_id).get_children_ids()) {
                if (m_tree->get_node(child_id).get_key_name() == *it) {
                    node_id = child_id;
                    found = true;
                    break;
                }
            }
            if (false == found) {
                return std::nullopt;
            }
        }

        for (auto schema_id : matched_schema_ids) {
            m_descriptor_to_schema[leaf_col.get()].emplace(schema_id, node_id);
        }
    }

    return std::make_tuple(true, std::move(leaves_expr));
}

auto SchemaMatch::decompose_and_match_log_message(std::string const& query)
        -> ClppDecompositionMatch {
    auto decomposed_query{decompose_query(std::nullopt, query, true)};
    if (decomposed_query.has_error()) {
        throw std::runtime_error(fmt::format("query decomposition failed for {}", query));
    }

    std::vector<clp_s::logtype_id_t> matched_lt_ids;
    for (auto const& log_type : m_archive_reader->get_typed_log_type_dictionary()->get_entries()) {
        if (decomposed_query.value()->get_log_type() == log_type.get_value()) {
            matched_lt_ids.emplace_back(log_type.get_id());
        }
    }
    return {decomposed_query.value(), std::move(matched_lt_ids)};
}

auto SchemaMatch::decompose_and_match_parent_var(
        SchemaNode const& cur_node,
        ast::DescriptorList::iterator cur_it,
        std::string const& query
) -> ClppDecompositionMatch {
    std::string type_name{cur_node.get_key_name()};
    auto const* parent_node{&m_tree->get_node(cur_node.get_parent_id())};
    size_t num_variable_descriptors{1};
    while (NodeType::LogMessage != parent_node->get_type()) {
        type_name.insert(0, parent_node->get_key_name());
        type_name.append(".");
        parent_node = &m_tree->get_node(parent_node->get_parent_id());
        ++num_variable_descriptors;
    }

    auto decomposed_query{decompose_query(type_name, query, 1 == num_variable_descriptors)};
    if (decomposed_query.has_error()) {
        throw std::runtime_error(fmt::format("query decomposition failed for {}", query));
    }

    std::vector<clp_s::logtype_id_t> matched_lt_ids;
    for (auto const& log_type : m_archive_reader->get_typed_log_type_dictionary()->get_entries()) {
        auto metadata{m_archive_reader->get_logtype_metadata().at(log_type.get_id())};
        for (auto const& parent_match : metadata.get_parent_matches()) {
            if (parent_match.m_name == cur_it->get_token()
                && decomposed_query.value()->get_log_type()
                           == log_type.get_value()
                                      .substr(parent_match.m_start, parent_match.m_size))
            {
                matched_lt_ids.emplace_back(log_type.get_id());
            }
        }
    }
    return {decomposed_query.value(), std::move(matched_lt_ids)};
}

auto SchemaMatch::decompose_query(
        std::optional<std::string> rule_name,
        std::string const& query,
        bool root_rule_query
) -> ystdlib::error_handling::Result<clpp::DecomposedQuery const*> {
    if (auto entry{m_decomposed_query_cache.find({rule_name, query})};
        m_decomposed_query_cache.end() != entry)
    {
        return &entry->second;
    }

    if (nullptr == m_ls_schema) {
        if (m_ls_schema = log_surgeon::log_surgeon_schema_from_definition(
                    log_surgeon::CCharArray::from_string_view(
                            YSTDLIB_ERROR_HANDLING_TRYX(m_archive_reader->read_log_surgeon_schema())
                    )
            );
            nullptr == m_ls_schema)
        {
            return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::BadParam};
        }
    }
    if (root_rule_query && nullptr == m_ls_parser) {
        m_ls_parser = std::make_unique<log_surgeon::ParserHandle>(m_ls_schema);
    }

    auto dq{(root_rule_query)
                    ? YSTDLIB_ERROR_HANDLING_TRYX(
                              clpp::DecomposedQuery::decompose_query(*m_ls_parser, rule_name, query)
                      )
                    : YSTDLIB_ERROR_HANDLING_TRYX(
                              clpp::DecomposedQuery::decompose_query(
                                      m_ls_schema,
                                      rule_name.value(),
                                      query
                              )
                      )};
    return &m_decomposed_query_cache.emplace(std::pair{rule_name, query}, std::move(dq))
                    .first->second;
}
}  // namespace clp_s::search
