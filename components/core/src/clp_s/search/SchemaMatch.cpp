#include "SchemaMatch.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <utils/profiling/ScopedProfiler.hpp>

#include <clp_s/archive_constants.hpp>
#include <clp_s/ArchiveReader.hpp>
#include <clp_s/Defs.hpp>
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
#include <clp_s/search/ast/NarrowSchemasExpr.hpp>
#include <clp_s/search/ast/OrExpr.hpp>
#include <clp_s/search/ast/OrOfAndForm.hpp>
#include <clp_s/search/ast/SearchUtils.hpp>
#include <clp_s/search/ast/ValueLiteral.hpp>
#include <clp_s/search/ClppMatcher.hpp>
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
 * One (interpretation index, occurrence index) combination that may hold for a schema: the index of
 * an interpretation in the matcher's `InterpretationMatch` list and the index of a same-node
 * StructurizedText occurrence (in document order) within the schema that the interpretation's
 * shape matched.
 */
using InterpretationOccurrence = std::pair<size_t, size_t>;

/**
 * The interpretations that may hold for each schema containing a decomposition root, split into
 * those that need their leaf queries checked and those that hold unconditionally.
 */
struct ApplicableInterpretations {
    // Each schema's applicable interpretation-occurrence pairs, in interpretation order and then
    // occurrence order. Excludes the unconditionally matched schemas.
    std::unordered_map<int32_t, std::vector<InterpretationOccurrence>> by_schema;
    // Schemas matched by an interpretation with no leaf constraints.
    std::unordered_set<int32_t> unconditionally_matched_schema_ids;
};

/**
 * Collects the interpretations applicable to each schema.
 * @param interpretations The interpretation matches to collect.
 * @return The applicable interpretations.
 */
[[nodiscard]] auto collect_applicable_interpretations(
        std::vector<ClppMatcher::InterpretationMatch> const& interpretations
) -> ApplicableInterpretations;

/**
 * Gets the `NodeType` corresponding to a given subtree type.
 * @param subtree_type
 * @return the corresponding `NodeType` or `NodeType::Unknown` if the subtree type is unknown.
 */
auto get_subtree_node_type(std::string_view subtree_type) -> NodeType;

/**
 * Adds `operand` to `expr`, splicing in `operand`'s operands instead when `operand` is itself a
 * non-inverted `ExprType`. This keeps `expr` in Or-of-And form, which evaluation relies on.
 * @tparam ExprType `AndExpr` or `OrExpr`; the type of `expr`.
 * @param expr
 * @param operand
 */
template <typename ExprType>
auto add_operand_flattened(
        std::shared_ptr<ast::Expression> const& expr,
        std::shared_ptr<ast::Expression> const& operand
) -> void;

/**
 * Determines whether a pure wildcard filter can be rewritten as per-node filters over decomposable
 * nodes. The following filters cannot:
 * - Columns that exclude decomposable nodes (already expanded on an earlier pass).
 * - NEXISTS and inverted EXISTS (an OR of per-node non-existence would be too permissive).
 * - Inverted filters whose operand is not a string (no per-node filter can be built, and the
 *   negated expansion requires one for every node).
 * @param column The pure wildcard column.
 * @param filter The filter containing `column`.
 * @return Whether `filter` can be rewritten as per-node filters over decomposable nodes.
 */
[[nodiscard]] auto is_expandable_over_decomposable_nodes(
        ast::ColumnDescriptor const& column,
        ast::FilterExpr const& filter
) -> bool;

/**
 * @param filter
 * @return `filter`'s operand converted to a string.
 * @return std::nullopt if the filter has no operand or the operand isn't a string under the
 * filter's operation.
 */
[[nodiscard]] auto operand_as_string(ast::FilterExpr const& filter) -> std::optional<std::string>;

/**
 * @param filter
 * @return Whether `filter` is an existence filter that is satisfied by a record in which its column
 * is absent, i.e. `NEXISTS` or inverted `EXISTS`.
 */
[[nodiscard]] auto existence_filter_holds_when_absent(ast::FilterExpr const& filter) -> bool;

/**
 * Determines whether `filter` must hold at every node its column resolves to within one schema
 * rather than at any one of them. A value filter holds if any node's value matches, so its
 * inversion holds only if no node's value matches; likewise NEXISTS and inverted EXISTS hold only
 * if the key is absent at every node.
 * @param filter
 * @return Whether `filter` must hold at every node.
 */
[[nodiscard]] auto filter_holds_at_every_node(ast::FilterExpr const& filter) -> bool;

/**
 * Determines whether `filter` is an existence filter on a pure wildcard column that can match every
 * type, i.e. the result of rewriting `*: *` or `NOT *: *`. Such a filter is decided without looking
 * at any column: `*` may match zero kv-pairs, so the EXISTS form holds for every record (including
 * one without any kv-pairs) and the NEXISTS form holds for none.
 * @param column The pure wildcard column.
 * @param filter The filter containing `column`.
 * @return Whether `filter` is an existence filter on an unrestricted pure wildcard column.
 */
[[nodiscard]] auto is_unrestricted_pure_wildcard_existence_filter(
        ast::ColumnDescriptor const& column,
        ast::FilterExpr const& filter
) -> bool;

/**
 * Copies `filter`, replacing its column with `column`. The operation, operand (if any) and
 * inversion are kept; the operand is shared with `filter`, not copied.
 * @param column
 * @param filter
 * @return The copied filter.
 */
[[nodiscard]] auto copy_filter_with_column(
        std::shared_ptr<ast::ColumnDescriptor> column,
        ast::FilterExpr const& filter
) -> std::shared_ptr<ast::Expression>;

/**
 * Narrows `common_schema` to `schemas`: replaces it when `first`, intersects otherwise.
 * @param schemas
 * @param common_schema
 * @param first Whether `common_schema` has not been initialized yet.
 */
auto narrow_common_schemas(std::set<int32_t> schemas, std::set<int32_t>& common_schema, bool first)
        -> void;

auto collect_applicable_interpretations(
        std::vector<ClppMatcher::InterpretationMatch> const& interpretations
) -> ApplicableInterpretations {
    ApplicableInterpretations applicable;
    for (size_t i{0}; i < interpretations.size(); ++i) {
        auto const& [occurrences, leaf_queries]{interpretations.at(i)};
        for (auto const& [schema_id, occurrence_indices] : occurrences) {
            if (leaf_queries.empty()) {
                applicable.unconditionally_matched_schema_ids.emplace(schema_id);
                continue;
            }
            for (auto const occurrence_idx : occurrence_indices) {
                applicable.by_schema[schema_id].emplace_back(i, occurrence_idx);
            }
        }
    }
    for (auto const schema_id : applicable.unconditionally_matched_schema_ids) {
        applicable.by_schema.erase(schema_id);
    }
    return applicable;
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

template <typename ExprType>
auto add_operand_flattened(
        std::shared_ptr<ast::Expression> const& expr,
        std::shared_ptr<ast::Expression> const& operand
) -> void {
    if (nullptr != std::dynamic_pointer_cast<ExprType>(operand) && false == operand->is_inverted())
    {
        ast::splice_into(expr, operand, expr->op_end());
        return;
    }
    expr->add_operand(operand);
}

auto is_expandable_over_decomposable_nodes(
        ast::ColumnDescriptor const& column,
        ast::FilterExpr const& filter
) -> bool {
    if (column.excludes_decomposable_nodes() || false == column.matches_type(LiteralType::StringT))
    {
        return false;
    }
    auto const op{filter.get_operation()};
    if (FilterOperation::NEXISTS == op) {
        return false;
    }
    if (filter.is_inverted()) {
        return FilterOperation::EXISTS != op && operand_as_string(filter).has_value();
    }
    return true;
}

auto operand_as_string(ast::FilterExpr const& filter) -> std::optional<std::string> {
    auto const operand{filter.get_operand()};
    if (nullptr == operand) {
        return std::nullopt;
    }
    std::string query;
    if (false == operand->as_var_string(query, filter.get_operation())
        && false == operand->as_clp_string(query, filter.get_operation()))
    {
        return std::nullopt;
    }
    return query;
}

auto existence_filter_holds_when_absent(ast::FilterExpr const& filter) -> bool {
    if (filter.is_inverted()) {
        return FilterOperation::EXISTS == filter.get_operation();
    }
    return FilterOperation::NEXISTS == filter.get_operation();
}

auto filter_holds_at_every_node(ast::FilterExpr const& filter) -> bool {
    return (FilterOperation::NEXISTS == filter.get_operation()) != filter.is_inverted();
}

auto is_unrestricted_pure_wildcard_existence_filter(
        ast::ColumnDescriptor const& column,
        ast::FilterExpr const& filter
) -> bool {
    auto const op{filter.get_operation()};
    return column.is_pure_wildcard() && column.matches_exactly(ast::cAllTypes)
           && (FilterOperation::EXISTS == op || FilterOperation::NEXISTS == op);
}

auto copy_filter_with_column(
        std::shared_ptr<ast::ColumnDescriptor> column,
        ast::FilterExpr const& filter
) -> std::shared_ptr<ast::Expression> {
    auto operand{filter.get_operand()};
    if (nullptr == operand) {
        return ast::FilterExpr::create(column, filter.get_operation(), filter.is_inverted());
    }
    return ast::FilterExpr::create(column, filter.get_operation(), operand, filter.is_inverted());
}

auto narrow_common_schemas(std::set<int32_t> schemas, std::set<int32_t>& common_schema, bool first)
        -> void {
    if (first) {
        common_schema = std::move(schemas);
        return;
    }
    std::erase_if(common_schema, [&](int32_t const schema_id) -> bool {
        return false == schemas.contains(schema_id);
    });
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
    PROFILE_SCOPE("schema_match");
    ConstantProp propagate_empty;
    expr = populate_column_mapping(expr);
    expr = propagate_empty.run(expr);
    if (std::dynamic_pointer_cast<EmptyExpr>(expr)) {
        return expr;
    }

    if (m_expr_rewritten) {
        m_column_to_descriptor.clear();

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

    split_expression_by_schema(expr, m_schema_to_expression, m_matched_schema_ids);

    auto const available_dictionaries{m_archive_reader->get_available_dictionaries()};
    auto required_dictionaries{ArchiveReader::DictionaryType::None};
    for (auto const schema_id : m_matched_schema_ids) {
        required_dictionaries |= get_required_dictionaries((*m_schemas)[schema_id].get_view());
        if (available_dictionaries == (required_dictionaries & available_dictionaries)) {
            break;
        }
    }
    m_archive_reader->read_dictionaries(required_dictionaries);

    return expr;
}

auto SchemaMatch::populate_column_mapping(std::shared_ptr<Expression> const& cur)
        -> std::shared_ptr<Expression> {
    for (auto it = cur->op_begin(); it != cur->op_end(); it++) {
        if (auto child = std::dynamic_pointer_cast<Expression>(*it)) {
            auto new_child = populate_column_mapping(child);
            if (new_child != child) {
                new_child->copy_replace(cur.get(), it);
            }
        } else if (auto const column{std::dynamic_pointer_cast<ast::ColumnDescriptor>(*it)};
                   nullptr != column)
        {
            auto resolved_expr{populate_column_mapping(column, cur)};
            if (nullptr == resolved_expr) {
                if (column->is_shape_query()) {
                    throw std::runtime_error{fmt::format(
                            "{}(<col>) can only be applied to StructurizedText or ParentRule "
                            "columns; no StructurizedText or ParentRule nodes match column \"{}\".",
                            ast::ColumnDescriptor::function_to_str(column->get_function()),
                            column->to_string()
                    )};
                }
                return EmptyExpr::create();
            }

            if (resolved_expr != cur) {
                return resolved_expr;
            }

            if (column->is_unresolved_descriptor() && false == column->is_pure_wildcard()
                && false == column->is_bound())
            {
                // TODO: will have to decide how we wan't to handle multi-column expressions
                // with unresolved descriptors
                return expand_unresolved_column(*column, dynamic_cast<FilterExpr const&>(*cur));
            }
        }
    }
    return cur;
}

auto SchemaMatch::expand_unresolved_column(
        ast::ColumnDescriptor const& column,
        ast::FilterExpr const& filter
) -> std::shared_ptr<Expression> {
    auto expanded{
            build_multi_node_filter(column, column.resolved_nodes().wildcard_node_ids, filter)
    };
    if (nullptr == expanded) {
        return EmptyExpr::create(filter.get_parent());
    }
    return expanded;
}

auto SchemaMatch::resolve_pure_wildcard(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        std::shared_ptr<ast::Expression> const& expr
) -> std::shared_ptr<ast::Expression> {
    auto const& filter{dynamic_cast<FilterExpr const&>(*expr)};
    if (is_unrestricted_pure_wildcard_existence_filter(*column, filter)) {
        return expr;
    }
    if (false == is_expandable_over_decomposable_nodes(*column, filter)) {
        return resolve_pure_wildcard_by_node_type(column, expr);
    }

    auto const decomposable_node_ids{collect_pure_wildcard_decomposable_node_ids(*column)};
    if (decomposable_node_ids.empty()) {
        if (column->is_shape_query()) {
            return nullptr;
        }
        return resolve_pure_wildcard_by_node_type(column, expr);
    }

    m_expr_rewritten = true;
    auto expanded{
            filter.is_inverted()
                    ? build_negated_pure_wildcard_expansion(*column, filter, decomposable_node_ids)
                    : build_pure_wildcard_expansion(*column, filter, decomposable_node_ids)
    };
    if (nullptr == expanded) {
        return EmptyExpr::create(expr->get_parent());
    }
    return expanded;
}

auto SchemaMatch::resolve_pure_wildcard_by_node_type(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        std::shared_ptr<ast::Expression> const& expr
) const -> std::shared_ptr<ast::Expression> {
    // TODO: This check is imprecise because it is archive-wide: the column is accepted when some
    // node anywhere in the selected subtrees has a matching type, without narrowing the schemas it
    // can match to those containing such a node. Consider removing it when we resolve issue #907
    // (pruning a pure wildcard that expands to no column of a schema during evaluation).
    if (pure_wildcard_matches_any_node_type(*column)) {
        return expr;
    }
    return nullptr;
}

auto SchemaMatch::pure_wildcard_matches_any_node_type(ast::ColumnDescriptor const& column) const
        -> bool {
    bool const excludes_decomposable_nodes{column.excludes_decomposable_nodes()};
    return std::ranges::any_of(
            select_pure_wildcard_subtree_roots(column),
            [&](mpt_node_id_t const subtree_root_node_id) -> bool {
                return walk_subtree(subtree_root_node_id, [&](SchemaNode const& node) -> bool {
                    if (excludes_decomposable_nodes && node.is_decomposable()) {
                        return false;
                    }
                    return column.matches_type(SchemaNode::node_to_literal_type(node.get_type()));
                });
            }
    );
}

auto SchemaMatch::collect_pure_wildcard_decomposable_node_ids(
        ast::ColumnDescriptor const& column
) const -> std::set<mpt_node_id_t> {
    std::set<mpt_node_id_t> decomposable_node_ids;
    for (auto const subtree_root_node_id : select_pure_wildcard_subtree_roots(column)) {
        walk_subtree(subtree_root_node_id, [&](SchemaNode const& node) -> bool {
            if (node.is_decomposable()) {
                decomposable_node_ids.emplace(node.get_id());
            }
            return false;
        });
    }
    return decomposable_node_ids;
}

auto SchemaMatch::select_pure_wildcard_subtree_roots(ast::ColumnDescriptor const& column) const
        -> std::vector<mpt_node_id_t> {
    // TODO: consider making pure wildcard resolution (and dynamic wildcard expansion in general)
    // respect namespaces. Ignoring them keeps a query against every field of a dataset with
    // several namespaces easy to write.
    return select_subtree_roots(column.get_subtree_type(), std::nullopt);
}

auto SchemaMatch::select_subtree_roots(
        std::optional<std::string> const& subtree_type,
        std::optional<std::string_view> required_namespace
) const -> std::vector<mpt_node_id_t> {
    std::optional<NodeType> required_type;
    if (subtree_type.has_value()) {
        required_type = get_subtree_node_type(subtree_type.value());
    }
    std::vector<mpt_node_id_t> subtree_root_node_ids;
    for (auto const& [namespace_and_type, subtree_root_node_id] : m_tree->get_subtrees()) {
        auto const& [subtree_namespace, type]{namespace_and_type};
        if (required_namespace.has_value() && required_namespace.value() != subtree_namespace) {
            continue;
        }
        bool const type_selected{
                required_type.has_value() ? required_type.value() == type
                                          : NodeType::Metadata != type
        };
        if (type_selected) {
            subtree_root_node_ids.push_back(subtree_root_node_id);
        }
    }
    return subtree_root_node_ids;
}

auto SchemaMatch::populate_column_mapping(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        std::shared_ptr<ast::Expression> const& expr
) -> std::shared_ptr<ast::Expression> {
    if (column->is_pure_wildcard()) {
        return resolve_pure_wildcard(column, expr);
    }

    column->resolved_nodes().wildcard_node_ids.clear();
    bool matched_node{false};
    std::set<mpt_node_id_t> leaf_node_ids;
    for (auto const subtree_root_node_id :
         select_subtree_roots(column->get_subtree_type(), column->get_namespace()))
    {
        for (auto const child_node_id : m_tree->get_node(subtree_root_node_id).get_children_ids()) {
            matched_node |= populate_column_mapping(column, child_node_id, leaf_node_ids);
        }
    }

    if (false == matched_node) {
        return nullptr;
    }
    bool const any_decomposable{
            std::ranges::any_of(leaf_node_ids, [&](mpt_node_id_t node_id) -> bool {
                return m_tree->get_node(node_id).is_decomposable();
            })
    };
    if (false == any_decomposable) {
        for (auto const node_id : leaf_node_ids) {
            record_column_at_node(column, node_id);
        }
        return expr;
    }

    m_expr_rewritten = true;
    auto expanded{
            build_multi_node_filter(*column, leaf_node_ids, dynamic_cast<FilterExpr const&>(*expr))
    };
    if (nullptr == expanded) {
        return EmptyExpr::create(expr->get_parent());
    }
    return expanded;
}

auto SchemaMatch::record_column_at_node(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        mpt_node_id_t node_id
) -> void {
    auto [descriptors_it, _]{m_column_to_descriptor.try_emplace(node_id)};
    descriptors_it->second.emplace(column);
}

auto SchemaMatch::build_multi_node_filter(
        ast::ColumnDescriptor const& column,
        std::set<mpt_node_id_t> const& node_ids,
        ast::FilterExpr const& filter
) -> std::shared_ptr<ast::Expression> {
    auto const op{filter.get_operation()};
    if (filter.is_inverted() && FilterOperation::EXISTS != op && FilterOperation::NEXISTS != op) {
        return build_negated_multi_node_filter(column, node_ids, filter);
    }
    bool const holds_when_absent{existence_filter_holds_when_absent(filter)};
    std::shared_ptr<Expression> combined{holds_when_absent ? AndExpr::create() : OrExpr::create()};
    for (auto const node_id : node_ids) {
        auto node_filter{build_resolved_node_filter(column, node_id, filter)};
        if (nullptr == node_filter) {
            if (holds_when_absent) {
                return nullptr;
            }
            continue;
        }
        combined->add_operand(node_filter);
    }
    if (combined->get_op_list().empty()) {
        return nullptr;
    }
    return combined;
}

auto SchemaMatch::build_negated_multi_node_filter(
        ast::ColumnDescriptor const& column,
        std::set<mpt_node_id_t> const& node_ids,
        ast::FilterExpr const& filter
) -> std::shared_ptr<ast::Expression> {
    auto groups{OrExpr::create()};
    for (auto const& [group_node_ids, schema_ids] : group_schemas_by_node_set(node_ids)) {
        if (group_node_ids.empty()) {
            continue;
        }
        auto group{AndExpr::create()};
        group->add_operand(narrow_to_schemas(schema_ids));
        bool any_node_filter_impossible{false};
        for (auto const node_id : group_node_ids) {
            auto negated_node_filter{build_resolved_node_filter(column, node_id, filter)};
            if (nullptr == negated_node_filter) {
                any_node_filter_impossible = true;
                break;
            }
            group->add_operand(negated_node_filter);
        }
        if (false == any_node_filter_impossible) {
            groups->add_operand(group);
        }
    }
    if (groups->get_op_list().empty()) {
        return nullptr;
    }
    return groups;
}

auto SchemaMatch::group_schemas_by_node_set(std::set<mpt_node_id_t> const& node_ids) const
        -> std::map<std::set<mpt_node_id_t>, std::unordered_set<int32_t>> {
    std::map<std::set<mpt_node_id_t>, std::unordered_set<int32_t>> schema_ids_by_node_ids;
    std::set<mpt_node_id_t> contained_node_ids;
    for (auto const& [schema_id, schema] : *m_schemas) {
        contained_node_ids.clear();
        auto const view{schema.get_view()};
        for (auto const node_id : node_ids) {
            if (view.any_node_id<true>([node_id](mpt_node_id_t const schema_node_id) -> bool {
                    return schema_node_id == node_id;
                }))
            {
                contained_node_ids.emplace(node_id);
            }
        }
        schema_ids_by_node_ids[contained_node_ids].emplace(schema_id);
    }
    return schema_ids_by_node_ids;
}

auto SchemaMatch::populate_column_mapping(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        mpt_node_id_t subtree_root_id,
        std::set<mpt_node_id_t>& leaf_node_ids
) -> bool {
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
    if (column->is_bound()) {
        if (false == column->is_bound_by_decomposition()) {
            record_column_at_node(column, column->get_bound_node_id());
        }
        return true;
    }
    auto it_start = column->descriptor_begin();
    work_list.emplace(0, it_start, subtree_root_id);
    // Allow matching a wildcard zero times
    if (column->descriptor_end() != it_start && it_start->wildcard()) {
        work_list.emplace(0, ++it_start, subtree_root_id);
    }
    int32_t prev_level = 0;
    bool matched_node{false};
    bool const is_shape_query{column->is_shape_query()};
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

        // Currently we only allow fully resolved, non-shape() descriptors for precise array search
        if (NodeType::UnstructuredArray == cur_node.get_type()
            && false == column->is_unresolved_descriptor() && false == is_shape_query)
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
            leaf_node_ids.emplace(cur_node_id);
            matched_node = true;
            continue;
        }
        bool const descriptor_consumed_at_node{
                is_key_name_empty ? at_descriptor_list_end : next_at_descriptor_list_end
        };
        if (descriptor_consumed_at_node
            && column->matches_type(SchemaNode::node_to_literal_type(cur_node.get_type())))
        {
            bool const is_decomposable{cur_node.is_decomposable()};
            if (is_shape_query && false == is_decomposable) {
                continue;
            }
            if (false == column->is_unresolved_descriptor()) {
                leaf_node_ids.emplace(cur_node_id);
            } else {
                column->resolved_nodes().wildcard_node_ids.emplace(cur_node_id);
                m_expr_rewritten = true;
            }
            matched_node = true;
            if (false == (wildcard_descriptor && is_decomposable)) {
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
    return matched_node;
}

void SchemaMatch::populate_schema_mapping() {
    // TODO: consider refactoring this to take advantage of the ordered region of the schema
    for (auto& it : *m_schemas) {
        int32_t schema_id = it.first;
        it.second.get_view().for_each_node_id([&](mpt_node_id_t node_id) -> void {
            if (false == m_column_to_descriptor.contains(node_id)) {
                return;
            }
            for (auto const& descriptor : m_column_to_descriptor.at(node_id)) {
                if (descriptor->is_pure_wildcard()) {
                    continue;
                }
                auto& node_ids{descriptor->resolved_nodes()
                                       .schema_to_node_ids.try_emplace(schema_id)
                                       .first->second};
                if (node_ids.end() == std::ranges::find(node_ids, node_id)) {
                    node_ids.push_back(node_id);
                }
            }
        });
    }
}

std::shared_ptr<Expression> SchemaMatch::intersect_schemas(std::shared_ptr<Expression> cur) {
    if (std::dynamic_pointer_cast<AndExpr>(cur) || std::dynamic_pointer_cast<FilterExpr>(cur)
        || std::dynamic_pointer_cast<ast::NarrowSchemasExpr>(cur))
    {
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

            for (auto const* column : columns) {
                if (column->is_pure_wildcard()) {
                    continue;
                }
                auto const& schema_to_node_ids{column->resolved_nodes().schema_to_node_ids};
                if (auto const node_ids_it{schema_to_node_ids.find(schema)};
                    schema_to_node_ids.end() != node_ids_it)
                {
                    m_schema_to_searched_columns[schema].insert(
                            node_ids_it->second.begin(),
                            node_ids_it->second.end()
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
    if (auto narrowing{std::dynamic_pointer_cast<ast::NarrowSchemasExpr>(cur)}) {
        narrow_common_schemas(get_narrowed_schemas(*narrowing), common_schema, first);
        return false;
    }

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
                narrow_common_schemas(
                        find_pure_wildcard_candidate_schemas(
                                *column,
                                dynamic_cast<FilterExpr const&>(*cur)
                        ),
                        common_schema,
                        first
                );
                return false;
            }

            auto const& cur_schemas{column->resolved_nodes().schema_to_node_ids};
            if (first) {
                if (FilterOperation::NEXISTS == op) {
                    for (auto const& schema : *m_schemas) {
                        if (is_nexists_candidate_schema(*column, schema.first)) {
                            common_schema.insert(schema.first);
                        }
                    }
                } else {
                    for (auto const& [schema_id, node_ids] : cur_schemas) {
                        common_schema.insert(schema_id);
                    }
                }
                return false;
            }
            if (FilterOperation::NEXISTS == op) {
                std::erase_if(common_schema, [&](int32_t const schema_id) -> bool {
                    return false == is_nexists_candidate_schema(*column, schema_id);
                });
            } else {
                std::erase_if(common_schema, [&](int32_t const schema_id) -> bool {
                    return false == cur_schemas.contains(schema_id);
                });
            }
        }
    }
    return first;
}

auto SchemaMatch::find_pure_wildcard_candidate_schemas(
        ast::ColumnDescriptor const& column,
        ast::FilterExpr const& filter
) const -> std::set<int32_t> {
    auto const op{filter.get_operation()};
    bool const holds_when_absent{existence_filter_holds_when_absent(filter)};
    std::set<int32_t> candidate_schemas;
    if (FilterOperation::EXISTS != op && FilterOperation::NEXISTS != op) {
        for (auto const& [schema_id, _] : *m_schemas) {
            candidate_schemas.insert(schema_id);
        }
        return candidate_schemas;
    }
    if (is_unrestricted_pure_wildcard_existence_filter(column, filter)) {
        if (false == holds_when_absent) {
            for (auto const& [schema_id, _] : *m_schemas) {
                candidate_schemas.insert(schema_id);
            }
        }
        return candidate_schemas;
    }

    auto const metadata_subtree_root_node_id{m_tree->get_metadata_subtree_node_id()};
    for (auto const& [schema_id, schema] : *m_schemas) {
        bool const has_matching_node{
                schema.get_view().any_node_id<true>([&](mpt_node_id_t node_id) -> bool {
                    auto const& node{m_tree->get_node(node_id)};
                    bool const is_metadata_column{
                            constants::cRootNodeId != metadata_subtree_root_node_id
                            && metadata_subtree_root_node_id == node.get_parent_id()
                    };
                    return false == is_metadata_column
                           && column.matches_type(
                                   SchemaNode::node_to_literal_type(node.get_type())
                           );
                })
        };
        if (has_matching_node != holds_when_absent) {
            candidate_schemas.insert(schema_id);
        }
    }
    return candidate_schemas;
}

auto SchemaMatch::is_nexists_candidate_schema(
        ast::ColumnDescriptor const& column,
        int32_t schema_id
) const -> bool {
    auto const& schema_to_node_ids{column.resolved_nodes().schema_to_node_ids};
    auto const node_ids_it{schema_to_node_ids.find(schema_id)};
    if (schema_to_node_ids.end() == node_ids_it) {
        return true;
    }
    return column.has_unresolved_tokens()
           && std::ranges::all_of(node_ids_it->second, [&](mpt_node_id_t const node_id) -> bool {
                  return NodeType::UnstructuredArray == m_tree->get_node(node_id).get_type();
              });
}

auto SchemaMatch::compute_candidate_types(
        ast::ColumnDescriptor const& column,
        std::set<int32_t> const& schemas
) const -> ast::literal_type_bitmask_t {
    if (column.is_bound_by_decomposition()) {
        return SchemaNode::node_to_literal_type(
                m_tree->get_node(column.get_bound_node_id()).get_type()
        );
    }
    auto const& schema_to_node_ids{column.resolved_nodes().schema_to_node_ids};
    ast::literal_type_bitmask_t types{0};
    for (auto const schema : schemas) {
        auto const node_ids_it{schema_to_node_ids.find(schema)};
        if (schema_to_node_ids.end() == node_ids_it) {
            continue;
        }
        for (auto const node_id : node_ids_it->second) {
            types |= SchemaNode::node_to_literal_type(m_tree->get_node(node_id).get_type());
        }
    }
    return types;
}

void SchemaMatch::split_expression_by_schema(
        std::shared_ptr<Expression> const& expr,
        std::map<int32_t, std::shared_ptr<Expression>>& schema_expressions,
        std::unordered_set<int32_t> const& relevant_schemas
) {
    if (std::dynamic_pointer_cast<ast::NarrowSchemasExpr>(expr)) {
        for (int32_t const schema_id : relevant_schemas) {
            schema_expressions[schema_id] = EmptyExpr::create_constant(true);
        }
    } else if (auto filter = std::dynamic_pointer_cast<FilterExpr>(expr)) {
        for (int32_t const schema_id : relevant_schemas) {
            schema_expressions[schema_id] = split_filter_for_schema(*filter, schema_id);
        }
    } else if (std::dynamic_pointer_cast<AndExpr>(expr)) {
        std::map<int32_t, std::shared_ptr<Expression>> sub_expressions;
        for (auto const& op : expr->get_op_list()) {
            auto sub_expr = std::static_pointer_cast<Expression>(op);
            split_expression_by_schema(sub_expr, sub_expressions, relevant_schemas);
            for (auto const& [schema_id, schema_sub_expr] : sub_expressions) {
                append_to_and(schema_expressions, schema_id, schema_sub_expr, expr->is_inverted());
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
            for (auto const& [schema_id, schema_sub_expr] : sub_expressions) {
                append_to_or(schema_expressions, schema_id, schema_sub_expr);
            }
            sub_expressions.clear();
        }

        if (expr->is_inverted()) {
            for (auto const& it : schema_expressions) {
                it.second->invert();
            }
        }
    }
}

auto SchemaMatch::append_to_and(
        std::map<int32_t, std::shared_ptr<ast::Expression>>& schema_expressions,
        int32_t schema_id,
        std::shared_ptr<ast::Expression> const& operand,
        bool inverted
) -> void {
    auto [expr_it, inserted]{schema_expressions.try_emplace(schema_id)};
    if (inserted) {
        expr_it->second = AndExpr::create(inverted);
    }
    add_operand_flattened<AndExpr>(expr_it->second, operand);
}

auto SchemaMatch::append_to_or(
        std::map<int32_t, std::shared_ptr<ast::Expression>>& schema_expressions,
        int32_t schema_id,
        std::shared_ptr<ast::Expression> const& operand
) -> void {
    auto [expr_it, inserted]{schema_expressions.try_emplace(schema_id)};
    if (inserted) {
        expr_it->second = operand;
        return;
    }
    auto& expr{expr_it->second};
    if (nullptr == std::dynamic_pointer_cast<OrExpr>(expr) || expr->is_inverted()) {
        auto or_expr{OrExpr::create()};
        or_expr->add_operand(expr);
        expr = or_expr;
    }
    add_operand_flattened<OrExpr>(expr, operand);
}

auto SchemaMatch::split_filter_for_schema(ast::FilterExpr const& filter, int32_t schema_id)
        -> std::shared_ptr<ast::Expression> {
    auto const& column{*filter.get_column()};
    auto const& schema_to_node_ids{column.resolved_nodes().schema_to_node_ids};
    auto const node_ids_it{schema_to_node_ids.find(schema_id)};
    if (schema_to_node_ids.end() == node_ids_it) {
        return copy_filter_with_column(column.copy_unbound(), filter);
    }
    auto const& node_ids{node_ids_it->second};
    auto const bind_filter_to_node{[&](mpt_node_id_t node_id) -> std::shared_ptr<Expression> {
        return copy_filter_with_column(
                column.copy_bound_to(
                        node_id,
                        SchemaNode::node_to_literal_type(m_tree->get_node(node_id).get_type())
                ),
                filter
        );
    }};
    if (1 == node_ids.size()) {
        return bind_filter_to_node(node_ids.front());
    }
    std::shared_ptr<Expression> combined{
            filter_holds_at_every_node(filter) ? AndExpr::create() : OrExpr::create()
    };
    for (auto const node_id : node_ids) {
        combined->add_operand(bind_filter_to_node(node_id));
    }
    return combined;
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

auto SchemaMatch::get_required_dictionaries(SchemaView const& schema) const
        -> ArchiveReader::DictionaryType {
    using DictionaryType = ArchiveReader::DictionaryType;
    auto dictionaries{DictionaryType::None};
    static_cast<void>(schema.any_node_id<true>([&](mpt_node_id_t node_id) -> bool {
        switch (m_tree->get_node(node_id).get_type()) {
            case NodeType::DictionaryFloat:
            case NodeType::VarString:
            case NodeType::UnstructurizedText:
                dictionaries |= DictionaryType::VarDict;
                break;
            case NodeType::ClpString:
                dictionaries |= DictionaryType::VarDict | DictionaryType::LogTypeDict;
                break;
            case NodeType::UnstructuredArray:
                dictionaries |= DictionaryType::VarDict | DictionaryType::ArrayDict;
                break;
            case NodeType::StructurizedText:
                dictionaries |= DictionaryType::LogShapeDict;
                break;
            case NodeType::ParentRule:
                dictionaries |= DictionaryType::ParentRuleShapes;
                break;
            default:
                break;
        }
        return false;
    }));
    return dictionaries;
}

auto SchemaMatch::release_schema_expressions() -> std::map<int32_t, std::shared_ptr<Expression>> {
    return std::exchange(m_schema_to_expression, {});
}

auto SchemaMatch::resolve_leaf_rule_descriptors(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        mpt_node_id_t decomposition_root,
        std::vector<std::string_view> const& rule_names
) -> std::optional<std::vector<std::pair<std::shared_ptr<ast::ColumnDescriptor>, mpt_node_id_t>>> {
    std::vector<std::string_view> parent_names;
    auto cur_id{decomposition_root};
    while (true) {
        auto const& node{m_tree->get_node(cur_id)};
        if (NodeType::StructurizedText == node.get_type()) {
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

    auto base_col{column->copy_unbound()};
    auto& base_descriptors{base_col->get_descriptor_list()};
    std::vector<mpt_node_id_t> cur_node_ids{decomposition_root};
    for (; seg_idx < rule_names.size(); ++seg_idx) {
        auto const token{rule_names.at(seg_idx)};
        base_descriptors.emplace_back(DescriptorToken::create_descriptor_from_literal_token(token));

        std::vector<mpt_node_id_t> next_node_ids;
        for (auto const node_id : cur_node_ids) {
            auto child_ids{find_child_nodes_by_key_name(node_id, token)};
            next_node_ids.insert(next_node_ids.end(), child_ids.begin(), child_ids.end());
        }
        if (next_node_ids.empty()) {
            return std::nullopt;
        }
        cur_node_ids = std::move(next_node_ids);
    }

    std::vector<std::pair<std::shared_ptr<ast::ColumnDescriptor>, mpt_node_id_t>> results;
    results.reserve(cur_node_ids.size());
    for (auto const node_id : cur_node_ids) {
        auto col{base_col->copy_unbound()};
        col->set_matching_types(
                SchemaNode::node_to_literal_type(m_tree->get_node(node_id).get_type())
        );
        results.emplace_back(std::move(col), node_id);
    }
    return results;
}

auto SchemaMatch::build_leaf_query_expr(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        mpt_node_id_t decomposition_root,
        std::span<clpp::LeafQuery const> leaf_queries,
        size_t occurrence_idx,
        std::unordered_set<int32_t> const& matched_schema_ids
) -> std::optional<std::shared_ptr<ast::Expression>> {
    if (leaf_queries.empty()) {
        auto narrowing{narrow_to_schemas(matched_schema_ids)};
        if (nullptr == narrowing) {
            return std::nullopt;
        }
        return narrowing;
    }
    auto leaves_expr{ast::AndExpr::create()};
    for (auto const& leaf : leaf_queries) {
        auto leaf_filters{build_leaf_filters(
                column,
                decomposition_root,
                leaf,
                occurrence_idx,
                matched_schema_ids,
                false
        )};
        if (false == leaf_filters.has_value()) {
            return std::nullopt;
        }
        auto type_variants{ast::OrExpr::create()};
        for (auto const& leaf_filter : leaf_filters.value()) {
            type_variants->add_operand(leaf_filter);
        }
        leaves_expr->add_operand(type_variants);
    }
    return leaves_expr;
}

auto SchemaMatch::build_negated_leaf_query_expr(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        mpt_node_id_t decomposition_root,
        std::span<clpp::LeafQuery const> leaf_queries,
        size_t occurrence_idx,
        std::unordered_set<int32_t> const& matched_schema_ids
) -> std::optional<std::shared_ptr<ast::Expression>> {
    auto any_leaf_fails{ast::OrExpr::create()};
    for (auto const& leaf : leaf_queries) {
        auto leaf_filters{build_leaf_filters(
                column,
                decomposition_root,
                leaf,
                occurrence_idx,
                matched_schema_ids,
                true
        )};
        if (false == leaf_filters.has_value()) {
            return std::nullopt;
        }
        auto no_type_variant_matches{ast::AndExpr::create()};
        for (auto const& leaf_filter : leaf_filters.value()) {
            no_type_variant_matches->add_operand(leaf_filter);
        }
        any_leaf_fails->add_operand(no_type_variant_matches);
    }
    return any_leaf_fails;
}

auto SchemaMatch::build_leaf_filters(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        mpt_node_id_t decomposition_root,
        clpp::LeafQuery const& leaf,
        size_t occurrence_idx,
        std::unordered_set<int32_t> const& matched_schema_ids,
        bool is_inverted
) -> std::optional<std::vector<std::shared_ptr<ast::Expression>>> {
    auto const rule_name{clpp::split_qualified_name(leaf.m_qualified_name)};
    auto leaf_cols{resolve_leaf_rule_descriptors(column, decomposition_root, rule_name)};
    if (false == leaf_cols.has_value()) {
        return std::nullopt;
    }
    std::vector<std::shared_ptr<ast::Expression>> filters;
    filters.reserve(leaf_cols->size());
    for (auto& [leaf_col, node_id] : leaf_cols.value()) {
        leaf_col->bind_as_decomposed_leaf(
                node_id,
                leaf.m_leaf_position,
                occurrence_idx,
                matched_schema_ids
        );
        auto leaf_literal{ast::ValueLiteral::create_from_string(leaf.m_query)};
        filters.emplace_back(
                FilterExpr::create(leaf_col, FilterOperation::EQ, leaf_literal, is_inverted)
        );
    }
    return filters;
}

auto SchemaMatch::find_child_nodes_by_key_name(mpt_node_id_t parent_id, std::string_view key_name)
        -> std::vector<mpt_node_id_t> {
    std::vector<mpt_node_id_t> result;
    for (auto child_id : m_tree->get_node(parent_id).get_children_ids()) {
        if (m_tree->get_node(child_id).get_key_name() == key_name) {
            result.push_back(child_id);
        }
    }
    return result;
}

auto SchemaMatch::build_shape_filter(
        mpt_node_id_t decomposition_root,
        std::string_view shape_query,
        bool is_inverted
) -> std::shared_ptr<ast::Expression> {
    auto const matched_schema_ids{m_clpp_matcher.find_matching_schemas(
            find_enclosing_structurized_text_node_id(decomposition_root),
            m_tree->build_ls_rule_name(decomposition_root),
            shape_query
    )};
    if (false == is_inverted) {
        return narrow_to_schemas(matched_schema_ids);
    }
    std::unordered_set<int32_t> unmatched_schema_ids;
    for (auto const schema_id : find_schemas_containing_node(decomposition_root)) {
        if (false == matched_schema_ids.contains(schema_id)) {
            unmatched_schema_ids.emplace(schema_id);
        }
    }
    return narrow_to_schemas(unmatched_schema_ids);
}

auto SchemaMatch::build_decomposed_query_filter(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        mpt_node_id_t decomposition_root,
        std::string_view rule_name,
        std::string_view query,
        bool is_inverted
) -> std::shared_ptr<ast::Expression> {
    auto interpretations{m_clpp_matcher.decompose_query(
            find_enclosing_structurized_text_node_id(decomposition_root),
            query,
            rule_name
    )};
    if (interpretations.has_error()) {
        throw std::runtime_error{fmt::format(
                "Failed to decompose query - ({}) {}",
                interpretations.error().category().name(),
                interpretations.error().message()
        )};
    }
    m_num_clpp_interpretations += interpretations.value().size();
    if (is_inverted) {
        return build_negated_decomposed_query_filter(
                column,
                decomposition_root,
                interpretations.value()
        );
    }
    auto results{ast::OrExpr::create()};
    for (auto const& [occurrences, leaf_queries] : interpretations.value()) {
        // Schemas can share a filter only when its same-node occurrence index agrees. Unconstrained
        // interpretations have no leaf filters to pin, so their schemas are collected together.
        std::map<size_t, std::unordered_set<int32_t>> schema_ids_by_occurrence;
        for (auto const& [schema_id, occurrence_indices] : occurrences) {
            for (auto const occurrence_idx : occurrence_indices) {
                schema_ids_by_occurrence[leaf_queries.empty() ? 0 : occurrence_idx].emplace(
                        schema_id
                );
            }
        }
        for (auto const& [occurrence_idx, schema_ids] : schema_ids_by_occurrence) {
            if (auto leaves_expr{build_leaf_query_expr(
                        column,
                        decomposition_root,
                        leaf_queries,
                        occurrence_idx,
                        schema_ids
                )};
                leaves_expr.has_value())
            {
                results->add_operand(*leaves_expr);
            }
        }
    }
    if (results->get_op_list().empty()) {
        return nullptr;
    }
    return results;
}

auto SchemaMatch::narrow_to_schemas(std::unordered_set<int32_t> const& schema_ids, bool is_inverted)
        -> std::shared_ptr<ast::Expression> {
    if (schema_ids.empty() && false == is_inverted) {
        return nullptr;
    }
    auto narrowing{ast::NarrowSchemasExpr::create(schema_ids)};
    if (is_inverted) {
        narrowing->invert();
    }
    return narrowing;
}

auto SchemaMatch::get_narrowed_schemas(ast::NarrowSchemasExpr const& narrowing) const
        -> std::set<int32_t> {
    auto const& schema_ids{narrowing.get_schema_ids()};
    if (false == narrowing.is_inverted()) {
        return {schema_ids.begin(), schema_ids.end()};
    }
    std::set<int32_t> complement;
    for (auto const& [schema_id, _] : *m_schemas) {
        if (false == schema_ids.contains(schema_id)) {
            complement.insert(schema_id);
        }
    }
    return complement;
}

auto SchemaMatch::find_schemas_containing_node(mpt_node_id_t node_id) const
        -> std::unordered_set<int32_t> {
    return m_clpp_matcher.find_matching_schemas(
            find_enclosing_structurized_text_node_id(node_id),
            m_tree->build_ls_rule_name(node_id),
            std::nullopt
    );
}

auto SchemaMatch::build_negated_decomposed_query_filter(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        mpt_node_id_t decomposition_root,
        std::vector<ClppMatcher::InterpretationMatch> const& interpretations
) -> std::shared_ptr<ast::Expression> {
    auto const applicable{collect_applicable_interpretations(interpretations)};

    std::unordered_set<int32_t> never_matched_schema_ids;
    for (auto const schema_id : find_schemas_containing_node(decomposition_root)) {
        if (false == applicable.unconditionally_matched_schema_ids.contains(schema_id)
            && false == applicable.by_schema.contains(schema_id))
        {
            never_matched_schema_ids.emplace(schema_id);
        }
    }

    auto results{ast::OrExpr::create()};
    auto add_narrowing{[&](std::unordered_set<int32_t> const& schema_ids) -> void {
        if (auto narrowing{narrow_to_schemas(schema_ids)}; nullptr != narrowing) {
            results->add_operand(narrowing);
        }
    }};
    add_narrowing(never_matched_schema_ids);

    std::map<std::vector<InterpretationOccurrence>, std::unordered_set<int32_t>>
            schema_ids_by_interpretation_group;
    for (auto const& [schema_id, interpretation_indices] : applicable.by_schema) {
        schema_ids_by_interpretation_group[interpretation_indices].emplace(schema_id);
    }
    for (auto const& [interpretation_indices, schema_ids] : schema_ids_by_interpretation_group) {
        auto no_interpretation_holds{ast::AndExpr::create()};
        for (auto const& [i, occurrence_idx] : interpretation_indices) {
            if (auto negated_expr{build_negated_leaf_query_expr(
                        column,
                        decomposition_root,
                        interpretations.at(i).leaf_queries,
                        occurrence_idx,
                        schema_ids
                )};
                negated_expr.has_value())
            {
                no_interpretation_holds->add_operand(*negated_expr);
            }
        }
        if (no_interpretation_holds->get_op_list().empty()) {
            add_narrowing(schema_ids);
            continue;
        }
        results->add_operand(no_interpretation_holds);
    }

    if (results->get_op_list().empty()) {
        return nullptr;
    }
    return results;
}

auto SchemaMatch::find_enclosing_structurized_text_node_id(mpt_node_id_t node_id) const
        -> mpt_node_id_t {
    return m_tree->find_matching_subtree_root_in_subtree(
            constants::cRootNodeId,
            node_id,
            NodeType::StructurizedText
    );
}

auto SchemaMatch::build_decomposable_node_filter(
        std::shared_ptr<ast::ColumnDescriptor> const& column,
        mpt_node_id_t decomposition_root,
        ast::FilterExpr const& filter
) -> std::shared_ptr<ast::Expression> {
    auto const op{filter.get_operation()};
    if (FilterOperation::EXISTS == op || FilterOperation::NEXISTS == op) {
        return narrow_to_schemas(
                find_schemas_containing_node(decomposition_root),
                existence_filter_holds_when_absent(filter)
        );
    }

    auto const operand_query{operand_as_string(filter)};
    if (false == operand_query.has_value()) {
        return nullptr;
    }

    std::string_view const query{operand_query.value()};
    if (column->is_shape_query()) {
        return build_shape_filter(decomposition_root, query, filter.is_inverted());
    }

    m_expr_rewritten = true;
    if (false == can_match_structurized_text(decomposition_root, filter, query)) {
        if (false == filter.is_inverted()) {
            return nullptr;
        }
        return narrow_to_schemas(find_schemas_containing_node(decomposition_root));
    }
    return build_decomposed_query_filter(
            column,
            decomposition_root,
            m_tree->build_ls_rule_name(decomposition_root),
            query,
            filter.is_inverted()
    );
}

auto SchemaMatch::can_match_structurized_text(
        mpt_node_id_t decomposition_root,
        ast::FilterExpr const& filter,
        std::string_view query
) -> bool {
    if (NodeType::StructurizedText != m_tree->get_node(decomposition_root).get_type()
        || filter.get_operand()->has_wildcard())
    {
        return true;
    }
    auto const should_structurize{m_clpp_matcher.should_structurize(query)};
    if (should_structurize.has_error()) {
        throw std::runtime_error{fmt::format(
                "Failed to read the archive's parsing spec - ({}) {}",
                should_structurize.error().category().name(),
                should_structurize.error().message()
        )};
    }
    return should_structurize.value();
}

auto SchemaMatch::build_resolved_decomposable_column(
        ast::ColumnDescriptor const& column,
        mpt_node_id_t node_id
) const -> std::shared_ptr<ast::ColumnDescriptor> {
    auto const* node{&m_tree->get_node(node_id)};
    auto const literal_type{SchemaNode::node_to_literal_type(node->get_type())};
    DescriptorList descriptors;
    while (constants::cRootNodeId != node->get_parent_id()) {
        descriptors.emplace_back(
                DescriptorToken::create_descriptor_from_literal_token(node->get_key_name())
        );
        node = &m_tree->get_node(node->get_parent_id());
    }
    std::reverse(descriptors.begin(), descriptors.end());
    auto resolved_column{ColumnDescriptor::create_from_descriptors(
            descriptors,
            node->get_key_name(),
            column.get_function()
    )};
    resolved_column->set_matching_type(literal_type);
    resolved_column->set_subtree_type(column.get_subtree_type());
    return resolved_column;
}

auto SchemaMatch::build_resolved_node_filter(
        ast::ColumnDescriptor const& column,
        mpt_node_id_t node_id,
        ast::FilterExpr const& filter
) -> std::shared_ptr<ast::Expression> {
    auto const& node{m_tree->get_node(node_id)};
    if (node.is_decomposable()) {
        auto resolved_column{build_resolved_decomposable_column(column, node_id)};
        return build_decomposable_node_filter(resolved_column, node_id, filter);
    }
    return copy_filter_with_column(
            column.copy_bound_to(node_id, SchemaNode::node_to_literal_type(node.get_type())),
            filter
    );
}

auto SchemaMatch::build_non_decomposable_wildcard_filter(
        ast::ColumnDescriptor const& column,
        ast::FilterExpr const& filter
) -> std::shared_ptr<ast::Expression> {
    if (column.is_shape_query()) {
        return nullptr;
    }
    auto non_decomposable_column{column.copy()};
    non_decomposable_column->set_excludes_decomposable_nodes();
    return copy_filter_with_column(std::move(non_decomposable_column), filter);
}

auto SchemaMatch::build_pure_wildcard_expansion(
        ast::ColumnDescriptor const& column,
        ast::FilterExpr const& filter,
        std::set<mpt_node_id_t> const& decomposable_node_ids
) -> std::shared_ptr<ast::Expression> {
    auto expansion{OrExpr::create()};
    if (auto non_decomposable_filter{build_non_decomposable_wildcard_filter(column, filter)};
        nullptr != non_decomposable_filter)
    {
        expansion->add_operand(non_decomposable_filter);
    }
    if (auto decomposable_filters{build_multi_node_filter(column, decomposable_node_ids, filter)};
        nullptr != decomposable_filters)
    {
        expansion->add_operand(decomposable_filters);
    }
    if (expansion->get_op_list().empty()) {
        return nullptr;
    }
    return expansion;
}

auto SchemaMatch::build_negated_pure_wildcard_expansion(
        ast::ColumnDescriptor const& column,
        ast::FilterExpr const& filter,
        std::set<mpt_node_id_t> const& decomposable_node_ids
) -> std::shared_ptr<ast::Expression> {
    auto groups{ast::OrExpr::create()};
    for (auto const& [node_ids, schema_ids] : group_schemas_by_node_set(decomposable_node_ids)) {
        if (auto group{build_negated_decomposable_node_group(column, filter, node_ids, schema_ids)};
            nullptr != group)
        {
            groups->add_operand(group);
        }
    }
    if (groups->get_op_list().empty()) {
        return nullptr;
    }
    return groups;
}

auto SchemaMatch::build_negated_decomposable_node_group(
        ast::ColumnDescriptor const& column,
        ast::FilterExpr const& filter,
        std::set<mpt_node_id_t> const& node_ids,
        std::unordered_set<int32_t> const& schema_ids
) -> std::shared_ptr<ast::Expression> {
    auto group{AndExpr::create()};
    if (auto narrowing{narrow_to_schemas(schema_ids)}; nullptr != narrowing) {
        group->add_operand(narrowing);
    }
    if (auto non_decomposable_filter{build_non_decomposable_wildcard_filter(column, filter)};
        nullptr != non_decomposable_filter)
    {
        group->add_operand(non_decomposable_filter);
    }
    for (auto const node_id : node_ids) {
        auto negated_node_filter{build_resolved_node_filter(column, node_id, filter)};
        if (nullptr == negated_node_filter) {
            return nullptr;
        }
        group->add_operand(negated_node_filter);
    }
    return group;
}
}  // namespace clp_s::search
