#include "SchemaMatch.hpp"

#include <algorithm>
#include <queue>
#include <string_view>
#include <tuple>
#include <utility>

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
auto get_subtree_node_type(std::string_view subtree_type) -> NodeType {
    if (constants::cMetadataSubtreeType == subtree_type) {
        return NodeType::Metadata;
    }
    if (constants::cObjectSubtreeType == subtree_type) {
        return NodeType::Object;
    }
    return NodeType::Unknown;
}
}  // namespace

// TODO: write proper iterators on the AST to make this code less awful.
// In particular schema intersection needs AST iterators and a proper refactor
SchemaMatch::SchemaMatch(
        std::shared_ptr<SchemaTree> tree,
        std::shared_ptr<ReaderUtils::SchemaMap> schemas
)
        : m_tree(std::move(tree)),
          m_schemas(std::move(schemas)) {}

std::shared_ptr<Expression> SchemaMatch::run(std::shared_ptr<Expression>& expr) {
    ConstantProp propagate_empty;
    expr = populate_column_mapping(expr);
    expr = propagate_empty.run(expr);
    if (std::dynamic_pointer_cast<EmptyExpr>(expr)) {
        return expr;
    }

    // if we had ambiguous column descriptors containing regex which were
    // resolved we need to restandardize the expression
    if (false == m_unresolved_descriptor_to_descriptor.empty()) {
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
            if (false == populate_column_mapping(column)) {
                // no matching columns -- replace this expression with empty;
                return EmptyExpr::create();
            } else if (column->is_unresolved_descriptor() && false == column->is_pure_wildcard()) {
                auto possibilities = OrExpr::create();

                // TODO: will have to decide how we wan't to handle multi-column expressions
                // with unresolved descriptors
                for (int32_t node_id : m_unresolved_descriptor_to_descriptor.at(column.get())) {
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

bool SchemaMatch::populate_column_mapping(std::shared_ptr<ast::ColumnDescriptor> const& column) {
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

        return matched;
    }

    auto resolve_against_subtree = [&](SchemaNode const& root_node) -> void {
        for (int32_t child_node_id : root_node.get_children_ids()) {
            matched |= populate_column_mapping(column, child_node_id);
        }
    };

    if (auto const& subtree_type{column->get_subtree_type()}; subtree_type.has_value()) {
        // Resolve against the subtree with matching namespace and type if it exists.
        auto const node_type{get_subtree_node_type(subtree_type.value())};
        if (auto const subtree_root_node_id
            = m_tree->get_subtree_node_id(column->get_namespace(), node_type);
            -1 != subtree_root_node_id)
        {
            auto const& root_node = m_tree->get_node(subtree_root_node_id);
            resolve_against_subtree(root_node);
        }
    } else {
        // Resolve against every subtree that has matching namespaces except for the
        // `NodeType::Metadata` subtree.
        for (auto const& [namespace_type_pair, subtree_root_node_id] : m_tree->get_subtrees()) {
            if (NodeType::Metadata != namespace_type_pair.second
                && namespace_type_pair.first == column->get_namespace())
            {
                auto const& root_node = m_tree->get_node(subtree_root_node_id);
                resolve_against_subtree(root_node);
            }
        }
    }
    return matched;
}

bool SchemaMatch::populate_column_mapping(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        int32_t node_id
) {
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
    return matched;
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
            if (false == m_column_to_descriptor.count(column_id)) {
                continue;
            }
            for (auto const& descriptor : m_column_to_descriptor[column_id]) {
                if (false == descriptor->is_pure_wildcard()) {
                    auto [schema_to_column_id_it, _]
                            = m_descriptor_to_schema.try_emplace(descriptor.get());
                    schema_to_column_id_it->second.emplace(schema_id, column_id);
                }
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
                if (m_descriptor_to_schema[column].count(schema)) {
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

bool SchemaMatch::intersect_and_sub_expr(
        std::shared_ptr<Expression> const& cur,
        std::set<int32_t>& common_schema,
        std::set<ColumnDescriptor*>& columns,
        bool first
) {
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
}  // namespace clp_s::search
