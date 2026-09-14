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
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <clp_s/archive_constants.hpp>
#include <clp_s/ArchiveReader.hpp>
#include <clp_s/Schema.hpp>
#include <clp_s/SchemaTree.hpp>
#include <clp_s/search/ast/AndExpr.hpp>
#include <clp_s/search/ast/ColumnDescriptor.hpp>
#include <clp_s/search/ast/ConstantProp.hpp>
#include <clp_s/search/ast/EmptyExpr.hpp>
#include <clp_s/search/ast/Expression.hpp>
#include <clp_s/search/ast/FilterExpr.hpp>
#include <clp_s/search/ast/FilterOperation.hpp>
#include <clp_s/search/ast/Literal.hpp>
#include <clp_s/search/ast/OrExpr.hpp>
#include <clp_s/search/ast/OrOfAndForm.hpp>
#include <clp_s/search/ast/SearchUtils.hpp>
#include <clp_s/search/ast/StringLiteral.hpp>
#include <clpp/Defs.hpp>
#include <clpp/Interpretation.hpp>

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
/**
 * Gets the `NodeType` corresponding to a given subtree type.
 * @param subtree_type
 * @return the corresponding `NodeType` or `NodeType::Unknown` if the subtree type is unknown.
 */
auto get_subtree_node_type(std::string_view subtree_type) -> NodeType;

auto get_subtree_node_type(std::string_view subtree_type) -> NodeType {
    if (constants::cMetadataSubtreeType == subtree_type) {
        return NodeType::Metadata;
    }
    if (constants::cObjectSubtreeType == subtree_type || clpp::cShapeFunction == subtree_type) {
        return NodeType::Object;
    }
    return NodeType::Unknown;
}
}  // namespace

// TODO: write proper iterators on the AST to make this code less awful.
// In particular schema intersection needs AST iterators and a proper refactor
SchemaMatch::SchemaMatch(std::shared_ptr<ArchiveReader> archive_reader, bool case_sensitive)
        : m_archive_reader(std::move(archive_reader)),
          m_tree(m_archive_reader->get_schema_tree()),
          m_schemas(m_archive_reader->get_schema_map()),
          m_clpp_matcher(m_archive_reader.get(), case_sensitive) {}

std::shared_ptr<Expression> SchemaMatch::run(std::shared_ptr<Expression>& expr) {
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

    for (auto const schema_id : m_matched_schema_ids) {
        read_dictionaries_for_schema((*m_schemas)[schema_id].get_view());
    }

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
            m_clpp_node_matched = false;
            auto [mapped_succesfully, new_and_expr]{populate_column_mapping(column, cur)};
            if (false == mapped_succesfully) {
                if (column->get_subtree_type().has_value()
                    && clpp::cShapeFunction == column->get_subtree_type().value()
                    && false == m_clpp_node_matched)
                {
                    throw std::runtime_error{fmt::format(
                            "{}(<col>) can only be applied to LogMessage or ParentRule columns; no "
                            "LogMessage or ParentRule nodes match column \"{}\".",
                            clpp::cShapeFunction,
                            ast::column_descriptor_to_string(*column)
                    )};
                }
                // no matching columns -- replace this expression with empty;
                return EmptyExpr::create();
            }

            if (new_and_expr != cur) {
                return new_and_expr;
            }

            if (column->is_unresolved_descriptor() && false == column->is_pure_wildcard()) {
                auto possibilities = OrExpr::create();

                // TODO: will have to decide how we wan't to handle multi-column expressions
                // with unresolved descriptors
                for (auto const node_id :
                     m_unresolved_descriptor_to_descriptor.at(column->get_id()))
                {
                    auto const* node{&m_tree->get_node(node_id)};
                    auto const matched_node_type{node->get_type()};
                    auto literal_type{SchemaNode::node_to_literal_type(matched_node_type)};
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
                    resolved_column->set_subtree_type(column->get_subtree_type());

                    auto const& filter{dynamic_cast<FilterExpr const&>(*cur.get())};
                    if (NodeType::LogMessage == matched_node_type
                        || NodeType::ParentRule == matched_node_type)
                    {
                        if (auto result{build_clpp_query_filter(resolved_column, node_id, filter)};
                            nullptr != result)
                        {
                            possibilities->add_operand(result);
                        }
                    } else if (FilterOperation::EXISTS == filter.get_operation()
                               || FilterOperation::NEXISTS == filter.get_operation())
                    {
                        auto resolved_filter{FilterExpr::create(
                                resolved_column,
                                filter.get_operation(),
                                filter.is_inverted()
                        )};
                        possibilities->add_operand(resolved_filter);
                    } else {
                        auto operand{filter.get_operand()};
                        auto resolved_filter{FilterExpr::create(
                                resolved_column,
                                filter.get_operation(),
                                operand,
                                filter.is_inverted()
                        )};
                        possibilities->add_operand(resolved_filter);
                    }
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
            if (column->matches_type(SchemaNode::node_to_literal_type(node.get_type()))) {
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
            auto [child_matched, new_expr]{populate_column_mapping(column, child_node_id, expr)};
            matched |= child_matched;
            if (new_expr != expr) {
                return {matched, std::move(new_expr)};
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
            auto [_, new_expr] = resolve_against_subtree(root_node);
            if (new_expr != expr) {
                return {matched, std::move(new_expr)};
            }
        }
    } else {
        // Resolve against every subtree that has matching namespaces except for the
        // `NodeType::Metadata` subtree.
        for (auto const& [namespace_type_pair, subtree_root_node_id] : m_tree->get_subtrees()) {
            if (NodeType::Metadata != namespace_type_pair.second
                && namespace_type_pair.first == column->get_namespace())
            {
                auto const& root_node = m_tree->get_node(subtree_root_node_id);
                auto [_, new_expr] = resolve_against_subtree(root_node);
                if (new_expr != expr) {
                    return {matched, std::move(new_expr)};
                }
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
    bool const is_shape_query{
            column->get_subtree_type().has_value()
            && clpp::cShapeFunction == column->get_subtree_type().value()
    };
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
                    && column->matches_type(SchemaNode::node_to_literal_type(cur_node.get_type()))))
        {
            bool const is_clpp_node{
                    NodeType::LogMessage == cur_node.get_type()
                    || NodeType::ParentRule == cur_node.get_type()
            };
            if (is_shape_query && false == is_clpp_node) {
                continue;
            }
            m_clpp_node_matched |= is_clpp_node;
            if (false == column->is_unresolved_descriptor()) {
                if (is_clpp_node) {
                    if (column->is_clpp_resolved()) {
                        matched = true;
                        continue;
                    }
                    if (auto result{build_clpp_query_filter(
                                column,
                                cur_node_id,
                                dynamic_cast<FilterExpr const&>(*expr.get())
                        )};
                        nullptr != result)
                    {
                        return std::make_tuple(true, std::move(result));
                    }
                    continue;
                }
                if (column->is_clpp_resolved()) {
                    matched = true;
                    continue;
                }
                auto [descriptors_it, _] = m_column_to_descriptor.try_emplace(cur_node_id);
                descriptors_it->second.emplace(column);
            } else {
                auto [node_ids_it, _]
                        = m_unresolved_descriptor_to_descriptor.try_emplace(column->get_id());
                node_ids_it->second.emplace(cur_node_id);
            }
            matched = true;
            if (false == (wildcard_descriptor && is_clpp_node)) {
                continue;
            }
        }

        // Allow matching a wildcard zero times
        if (false == next_at_descriptor_list_end && next_it->wildcard()) {
            work_list.emplace(cur_depth, next_it, cur_node_id);
        }

        // Push nodes to the work list
        for (int32_t child_node_id : cur_node.get_children_ids()) {
            if (is_key_name_empty) {
                // Don't advance the iterator when accepting an empty key
                work_list.emplace(cur_depth + 1, cur_it, child_node_id);
            } else {
                work_list.emplace(cur_depth + 1, next_it, child_node_id);
                if (wildcard_descriptor) {
                    // Allow matching a wildcard token multiple times
                    work_list.emplace(cur_depth + 1, cur_it, child_node_id);
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
        it.second.get_view().for_each_node_id([&](SchemaNode::id_t column_id) -> void {
            if (false == m_column_to_descriptor.contains(column_id)) {
                return;
            }
            for (auto const& descriptor : m_column_to_descriptor.at(column_id)) {
                if (descriptor->is_clpp_resolved() || descriptor->is_pure_wildcard()) {
                    continue;
                }

                auto [schema_to_column_id_it, _]
                        = m_descriptor_to_schema.try_emplace(descriptor->get_id());
                schema_to_column_id_it->second.emplace(schema_id, column_id);
            }
        });
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

        for (auto* column : columns) {
            if (column->is_pure_wildcard()) {
                continue;
            }

            column->set_matching_types(compute_candidate_types(*column, common_schema));
        }

        for (int32_t schema : common_schema) {
            m_matched_schema_ids.insert(schema);

            for (auto column : columns) {
                if (false == column->is_pure_wildcard()) {
                    m_schema_to_searched_columns[schema].insert(
                            get_column_id_for_descriptor(column->get_id(), schema)
                    );
                }
            }
        }
    } else if (auto or_expr{std::dynamic_pointer_cast<OrExpr>(cur)}) {
        for (auto it{or_expr->op_begin()}; it != or_expr->op_end(); ++it) {
            auto sub_expr{std::static_pointer_cast<Expression>(*it)};
            auto new_expr{intersect_schemas(sub_expr)};
            if (new_expr != sub_expr) {
                *it = new_expr;
            }
        }

        std::set<int32_t> or_schemas;
        std::set<ColumnDescriptor*> or_columns;
        for (auto const& op : or_expr->get_op_list()) {
            auto sub_expr{std::dynamic_pointer_cast<Expression>(op)};
            if (nullptr == sub_expr) {
                continue;
            }
            if (auto it{m_expression_to_schemas.find(sub_expr.get())};
                m_expression_to_schemas.end() != it)
            {
                for (int32_t schema_id : it->second) {
                    or_schemas.insert(schema_id);
                }
            }
            if (auto filter{std::dynamic_pointer_cast<FilterExpr>(op)}) {
                auto* col{filter->get_column().get()};
                if (false == col->is_pure_wildcard()) {
                    or_columns.insert(col);
                }
            }
        }

        if (or_schemas.empty()) {
            return EmptyExpr::create(cur->get_parent());
        }

        for (int32_t schema_id : or_schemas) {
            m_expression_to_schemas[cur.get()].insert(schema_id);
        }

        for (auto* column : or_columns) {
            if (column->is_pure_wildcard()) {
                continue;
            }
            column->set_matching_types(compute_candidate_types(*column, or_schemas));
        }

        for (int32_t schema : or_schemas) {
            m_matched_schema_ids.insert(schema);
            for (auto column : or_columns) {
                if (false == column->is_pure_wildcard()
                    && m_descriptor_to_schema.contains(column->get_id())
                    && m_descriptor_to_schema.at(column->get_id()).contains(schema))
                {
                    m_schema_to_searched_columns[schema].insert(
                            get_column_id_for_descriptor(column->get_id(), schema)
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
            if (auto or_expr{std::dynamic_pointer_cast<OrExpr>(sub_expr)}) {
                std::set<int32_t> or_schemas;
                for (auto const& or_op : or_expr->get_op_list()) {
                    if (auto or_filter{std::dynamic_pointer_cast<FilterExpr>(or_op)}) {
                        auto* or_col{or_filter->get_column().get()};
                        if (or_col->is_pure_wildcard()) {
                            for (auto const& schema_it : *m_schemas) {
                                or_schemas.insert(schema_it.first);
                            }
                        } else if (m_descriptor_to_schema.contains(or_col->get_id())) {
                            auto const& col_schemas{m_descriptor_to_schema.at(or_col->get_id())};
                            for (auto const& [schema_id, _] : col_schemas) {
                                or_schemas.insert(schema_id);
                            }
                        }
                        if (FilterOperation::EXISTS != or_filter->get_operation()
                            && FilterOperation::NEXISTS != or_filter->get_operation())
                        {
                            columns.insert(or_col);
                        }
                    }
                }

                if (first) {
                    common_schema = or_schemas;
                } else {
                    std::set<int32_t> intersection;
                    for (int32_t schema : common_schema) {
                        if (or_schemas.contains(schema)) {
                            intersection.insert(schema);
                        }
                    }
                    common_schema = intersection;
                }
                first = false;
                if (common_schema.empty()) {
                    break;
                }
            } else {
                first &= intersect_and_sub_expr(sub_expr, common_schema, columns, first);
                if (false == first && common_schema.empty()) {
                    break;
                }
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
                for (auto const& [schema_id, column_id] :
                     m_descriptor_to_schema.at(column->get_id()))
                {
                    common_schema.insert(schema_id);
                }
                return false;
            } else if (first /*&& op == FilterOperation::NEXISTS */) {
                auto const& cur_schemas{m_descriptor_to_schema.at(column->get_id())};
                for (auto& schema : *m_schemas) {
                    if (0 == cur_schemas.count(schema.first)) {
                        common_schema.insert(schema.first);
                    }
                }
                return false;
            } else if (op == FilterOperation::NEXISTS) {
                std::set<int32_t> intersection;
                auto const& cur_schemas{m_descriptor_to_schema.at(column->get_id())};
                for (int32_t schema : common_schema) {
                    if (0 == cur_schemas.count(schema)) {
                        intersection.insert(schema);
                    }
                }
                common_schema = intersection;
            } else {
                std::set<int32_t> intersection;
                auto const& cur_schemas{m_descriptor_to_schema.at(column->get_id())};
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

auto SchemaMatch::compute_candidate_types(
        ast::ColumnDescriptor const& column,
        std::set<int32_t> const& schemas
) const -> ast::literal_type_bitmask_t {
    auto const& schema_mappings{m_descriptor_to_schema.at(column.get_id())};
    ast::literal_type_bitmask_t types{0};
    if (column.is_clpp_resolved()) {
        for (auto const& [schema_id, node_id] : schema_mappings) {
            types |= SchemaNode::node_to_literal_type(m_tree->get_node(node_id).get_type());
        }
        return types;
    }
    for (auto const schema : schemas) {
        if (schema_mappings.contains(schema)) {
            types |= SchemaNode::node_to_literal_type(
                    m_tree->get_node(schema_mappings.at(schema)).get_type()
            );
        }
    }
    return types;
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
            auto old_descriptor_id{filter->get_column()->get_id()};

            if (false == descriptor->is_pure_wildcard()) {
                descriptor->set_column_id(
                        get_column_id_for_descriptor(old_descriptor_id, schema_id)
                );
                auto literal_type = get_literal_type_for_column(old_descriptor_id, schema_id);
                if (literal_type == LiteralType::ArrayT) {
                    m_array_search_schema_ids.insert(schema_id);
                }
                descriptor->set_matching_type(literal_type);
            } else if ((descriptor->is_pure_wildcard()
                        && descriptor->matches_type(LiteralType::ArrayT)
                        && 0 == m_array_search_schema_ids.count(schema_id)))
            {
                if ((*m_schemas)[schema_id].get_view().any_node_id(
                            [&](SchemaNode::id_t column_id) -> bool {
                                return NodeType::UnstructuredArray
                                       == m_tree->get_node(column_id).get_type();
                            }
                    ))
                {
                    m_array_search_schema_ids.insert(schema_id);
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

int32_t SchemaMatch::get_column_id_for_descriptor(ColumnDescriptor::id_t col_id, int32_t schema) {
    return m_descriptor_to_schema.at(col_id).at(schema);
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

auto SchemaMatch::read_dictionaries_for_schema(SchemaView const& schema) -> void {
    schema.visit_entries(
            [&](SchemaNode::id_t column_id) -> bool {
                switch (m_tree->get_node(column_id).get_type()) {
                    case NodeType::DictionaryFloat:
                    case NodeType::VarString:
                        m_archive_reader->get_variable_dictionary();
                        break;
                    case NodeType::ClpString:
                        m_archive_reader->get_variable_dictionary();
                        m_archive_reader->get_log_type_dictionary();
                        break;
                    case NodeType::UnstructuredArray:
                        m_archive_reader->get_variable_dictionary();
                        m_archive_reader->get_array_dictionary();
                        break;
                    default:
                        break;
                }
                return false;
            },
            [&](UnorderedObject const& obj) -> bool {
                if (NodeType::LogMessage == obj.type) {
                    m_archive_reader->get_log_shape_dictionary();
                } else if (NodeType::ParentRule == obj.type) {
                    m_archive_reader->get_parent_rule_shapes();
                }
                read_dictionaries_for_schema(obj.sub_schema);
                return false;
            }
    );
}

LiteralType
SchemaMatch::get_literal_type_for_column(ColumnDescriptor::id_t col_id, int32_t schema) {
    return SchemaNode::node_to_literal_type(
            m_tree->get_node(get_column_id_for_descriptor(col_id, schema)).get_type()
    );
}

std::shared_ptr<Expression> SchemaMatch::get_query_for_schema(int32_t schema) {
    return m_schema_to_query.at(schema);
}

auto SchemaMatch::resolve_leaf_rule_descriptors(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        SchemaNode::id_t root_node_id,
        std::vector<std::string_view> const& rule_names
) -> std::
        optional<std::vector<std::pair<std::shared_ptr<ast::ColumnDescriptor>, SchemaNode::id_t>>> {
    std::vector<std::string_view> parent_names;
    auto cur_id{root_node_id};
    while (true) {
        auto const& node{m_tree->get_node(cur_id)};
        if (NodeType::LogMessage == node.get_type()) {
            break;
        }
        parent_names.emplace_back(node.get_key_name());
        cur_id = node.get_parent_id();
    }
    std::reverse(parent_names.begin(), parent_names.end());

    size_t seg_idx{0};
    for (size_t path_idx{0}; path_idx < parent_names.size() && seg_idx < rule_names.size();
         ++path_idx)
    {
        if (parent_names[path_idx] == rule_names.at(seg_idx)) {
            ++seg_idx;
        } else {
            break;
        }
    }

    auto base_col{column->copy_with_new_id()};
    auto& base_descriptors{base_col->get_descriptor_list()};
    std::vector<SchemaNode::id_t> cur_node_ids{root_node_id};
    for (; seg_idx < rule_names.size(); ++seg_idx) {
        auto const token{rule_names.at(seg_idx)};
        base_descriptors.emplace_back(DescriptorToken::create_descriptor_from_literal_token(token));

        std::vector<SchemaNode::id_t> next_node_ids;
        for (auto const node_id : cur_node_ids) {
            auto child_ids{find_child_nodes_by_key_name(node_id, token)};
            next_node_ids.insert(next_node_ids.end(), child_ids.begin(), child_ids.end());
        }
        if (next_node_ids.empty()) {
            return std::nullopt;
        }
        cur_node_ids = std::move(next_node_ids);
    }

    std::vector<std::pair<std::shared_ptr<ast::ColumnDescriptor>, SchemaNode::id_t>> results;
    results.reserve(cur_node_ids.size());
    for (auto const node_id : cur_node_ids) {
        auto col{base_col->copy_with_new_id()};
        col->set_matching_types(
                SchemaNode::node_to_literal_type(m_tree->get_node(node_id).get_type())
        );
        results.emplace_back(std::move(col), node_id);
    }
    return results;
}

auto SchemaMatch::register_clpp_resolved_column(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        SchemaNode::id_t node_id,
        std::unordered_set<int32_t> const& matched_schema_ids
) -> void {
    column->set_clpp_resolved(true);
    auto& mappings{m_descriptor_to_schema[column->get_id()]};
    for (int32_t const schema_id : matched_schema_ids) {
        mappings.emplace(schema_id, node_id);
    }
}

auto SchemaMatch::build_leaf_query_expr(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        SchemaNode::id_t root_node_id,
        clpp::Interpretation const& interpretation,
        std::unordered_set<int32_t> const& matched_schema_ids
) -> std::optional<std::shared_ptr<ast::Expression>> {
    if (interpretation.m_leaf_queries.empty()) {
        auto col{column->copy_with_new_id()};
        register_clpp_resolved_column(col, root_node_id, matched_schema_ids);
        return FilterExpr::create(col, FilterOperation::EXISTS);
    }
    auto leaves_expr{ast::AndExpr::create()};
    for (auto const& leaf : interpretation.m_leaf_queries) {
        auto rule_names{clpp::split_qualified_name(leaf.m_qualified_name)};
        auto leaf_cols{resolve_leaf_rule_descriptors(column, root_node_id, rule_names)};
        if (false == leaf_cols.has_value()) {
            return std::nullopt;
        }

        auto type_variants{ast::OrExpr::create()};
        for (auto& [new_col, node_id] : leaf_cols.value()) {
            register_clpp_resolved_column(new_col, node_id, matched_schema_ids);
            if ("*" == leaf.m_query) {
                type_variants->add_operand(
                        FilterExpr::create(new_col, ast::FilterOperation::EXISTS)
                );
            } else {
                auto leaf_literal{ast::StringLiteral::create(leaf.m_query)};
                type_variants->add_operand(
                        FilterExpr::create(new_col, ast::FilterOperation::EQ, leaf_literal)
                );
            }
        }
        leaves_expr->add_operand(type_variants);
    }
    return leaves_expr;
}

auto
SchemaMatch::find_child_nodes_by_key_name(SchemaNode::id_t parent_id, std::string_view key_name)
        -> std::vector<SchemaNode::id_t> {
    std::vector<SchemaNode::id_t> result;
    for (auto child_id : m_tree->get_node(parent_id).get_children_ids()) {
        if (m_tree->get_node(child_id).get_key_name() == key_name) {
            result.push_back(child_id);
        }
    }
    return result;
}

auto SchemaMatch::build_shape_match_filter(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        SchemaNode::id_t root_node_id,
        std::string_view rule_name,
        std::optional<std::string_view> shape_query,
        FilterOperation op,
        bool is_inverted
) -> std::shared_ptr<ast::Expression> {
    auto const matched_schema_ids{m_clpp_matcher.find_matching_schemas(rule_name, shape_query)};
    if (matched_schema_ids.empty()) {
        return nullptr;
    }
    auto clpp_column{column->copy_with_new_id()};
    register_clpp_resolved_column(clpp_column, root_node_id, matched_schema_ids);
    return FilterExpr::create(clpp_column, op, is_inverted);
}

auto SchemaMatch::build_decomposed_query_filter(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        SchemaNode::id_t root_node_id,
        std::string_view rule_name,
        std::string_view query
) -> std::shared_ptr<ast::Expression> {
    auto results{ast::OrExpr::create()};
    auto interpretations{m_clpp_matcher.decompose_query(query, rule_name)};
    if (interpretations.has_error()) {
        throw std::runtime_error{fmt::format(
                "Failed to decompose query - ({}) {}",
                interpretations.error().category().name(),
                interpretations.error().message()
        )};
    }
    for (auto const& [schema_ids, interpretation] : interpretations.value()) {
        ++m_num_clpp_interpretations;
        if (auto leaves_expr{
                    build_leaf_query_expr(column, root_node_id, interpretation, schema_ids)
            };
            leaves_expr.has_value())
        {
            results->add_operand(*leaves_expr);
        }
    }
    if (results->get_op_list().empty()) {
        return nullptr;
    }
    return results;
}

auto SchemaMatch::build_clpp_query_filter(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        SchemaNode::id_t root_node_id,
        ast::FilterExpr const& filter
) -> std::shared_ptr<ast::Expression> {
    auto const rule_name{m_tree->build_ls_rule_name(root_node_id)};

    auto const op{filter.get_operation()};
    if (FilterOperation::EXISTS == op || FilterOperation::NEXISTS == op) {
        return build_shape_match_filter(
                column,
                root_node_id,
                rule_name,
                std::nullopt,
                op,
                filter.is_inverted()
        );
    }

    auto const operand{filter.get_operand()};
    if (nullptr == operand) {
        return nullptr;
    }
    std::string query;
    if (false == operand->as_var_string(query, op)) {
        return nullptr;
    }

    if (column->get_subtree_type().has_value()
        && clpp::cShapeFunction == column->get_subtree_type().value())
    {
        return build_shape_match_filter(
                column,
                root_node_id,
                rule_name,
                query,
                FilterOperation::EXISTS,
                filter.is_inverted()
        );
    }

    m_clpp_decomposed_query = true;
    return build_decomposed_query_filter(column, root_node_id, rule_name, query);
}
}  // namespace clp_s::search
