#include "utils.hpp"

#include <bitset>
#include <cstddef>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/core.h>
#include <fmt/format.h>
#include <ystdlib/error_handling/Result.hpp>

#include "../../../../../clp_s/search/ast/Literal.hpp"
#include "../../../SchemaTree.hpp"

namespace clp::ffi::ir_stream::search::test {
namespace {
using clp_s::search::ast::LiteralType;
}  // namespace

auto ColumnQueryPossibleMatches::get_matchable_node_ids_from_literal_type(LiteralType type) const
        -> std::vector<SchemaTree::Node::id_t> {
    if ((type & m_matchable_types) == 0) {
        return {};
    }

    switch (type) {
        case LiteralType::IntegerT:
            return get_matchable_node_ids_from_schema_tree_type(SchemaTree::Node::Type::Int);
        case LiteralType::FloatT:
            return get_matchable_node_ids_from_schema_tree_type(SchemaTree::Node::Type::Float);
        case LiteralType::BooleanT:
            return get_matchable_node_ids_from_schema_tree_type(SchemaTree::Node::Type::Bool);
        case LiteralType::VarStringT:
        case LiteralType::ClpStringT:
            return get_matchable_node_ids_from_schema_tree_type(SchemaTree::Node::Type::Str);
        default:
            return {};
    }
}

auto ColumnQueryPossibleMatches::get_matchable_node_ids_from_schema_tree_type(
        SchemaTree::Node::Type type
) const -> std::vector<SchemaTree::Node::id_t> {
    std::vector<SchemaTree::Node::id_t> node_ids;
    for (auto const node_id : m_matchable_node_ids) {
        if (m_schema_tree->get_node(node_id).get_type() == type) {
            node_ids.emplace_back(node_id);
        }
    }
    return node_ids;
}

auto ColumnQueryPossibleMatches::serialize() const -> std::string {
    constexpr size_t cBitsetSize{sizeof(m_matchable_types) * 8};
    return fmt::format(
            "Types: {}; Node IDs: {{{}}}",
            std::bitset<cBitsetSize>(m_matchable_types).to_string(),
            fmt::join(m_matchable_node_ids, ",")
    );
}

auto trivial_new_projected_schema_tree_node_callback(
        [[maybe_unused]] bool is_auto_generated,
        [[maybe_unused]] SchemaTree::Node::id_t node_id,
        [[maybe_unused]] std::string_view projected_key_path
) -> ystdlib::error_handling::Result<void> {
    return ystdlib::error_handling::success();
}

auto get_schema_tree_column_queries(std::shared_ptr<SchemaTree> const& schema_tree)
        -> std::map<std::string, ColumnQueryPossibleMatches> {
    std::map<std::string, ColumnQueryPossibleMatches> column_query_to_possible_matches;
    std::vector<std::pair<std::string, SchemaTree::Node::id_t>> search_stack;

    auto push_all_children_to_stack
            = [&](SchemaTree::Node const& node, std::string const& query) -> void {
        for (auto const node_id : node.get_children_ids()) {
            search_stack.emplace_back(query, node_id);
        }
    };

    push_all_children_to_stack(schema_tree->get_root(), "");
    push_all_children_to_stack(schema_tree->get_root(), "*");

    // For every node, there are two possible input column queries:
    // 1. [...].[*]
    // 2. [...].[parent_key]
    // Based on this input, there are five valid column queries for each node:
    // 1. [...].[*]
    // 2. [...].[*].[curr_key]
    // 3. [...].[*].[curr_key].[*]
    // 4. [...].[parent_key].[curr_key]
    // 5. [...].[parent_key].[curr_key].[*]

    while (false == search_stack.empty()) {
        auto const [parent_query, node_id] = search_stack.back();
        search_stack.pop_back();

        auto const& node{schema_tree->get_node(node_id)};
        bool const is_parent_query_empty{parent_query.empty()};
        bool const parent_query_end_with_wildcard{
                is_parent_query_empty ? false : parent_query.back() == '*'
        };

        if (parent_query_end_with_wildcard) {
            // Handle the trailing wildcard from the parent node
            auto [it, inserted] = column_query_to_possible_matches.try_emplace(
                    parent_query,
                    ColumnQueryPossibleMatches{schema_tree}
            );
            it->second.set_matchable_node(node_id, node.get_type());
            push_all_children_to_stack(node, parent_query);
        }

        auto const query_with_curr_key{
                parent_query + (is_parent_query_empty ? "" : ".") + std::string{node.get_key_name()}
        };
        auto [without_wildcard_it, without_wildcard_inserted]
                = column_query_to_possible_matches.try_emplace(
                        query_with_curr_key,
                        ColumnQueryPossibleMatches{schema_tree}
                );
        without_wildcard_it->second.set_matchable_node(node_id, node.get_type());
        push_all_children_to_stack(node, query_with_curr_key);

        auto const query_with_trailing_wildcard{query_with_curr_key + ".*"};
        auto [with_wildcard_it, with_wildcard_inserted]
                = column_query_to_possible_matches.try_emplace(
                        query_with_trailing_wildcard,
                        ColumnQueryPossibleMatches{schema_tree}
                );
        with_wildcard_it->second.set_matchable_node(node_id, node.get_type());
        push_all_children_to_stack(node, query_with_trailing_wildcard);
    }

    return column_query_to_possible_matches;
}

auto operator<<(
        std::ostream& os,
        std::map<std::string, ColumnQueryPossibleMatches> const& column_query_to_possible_matches
) -> std::ostream& {
    for (auto const& [query, possible_matches] : column_query_to_possible_matches) {
        os << fmt::format("Query: {}; {}\n", query, possible_matches.serialize());
    }
    return os;
}
}  // namespace clp::ffi::ir_stream::search::test
