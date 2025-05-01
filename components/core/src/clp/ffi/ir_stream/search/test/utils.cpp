#include "utils.hpp"

#include <bitset>
#include <map>
#include <ostream>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "../../../SchemaTree.hpp"

namespace clp::ffi::ir_stream::search::test {
auto PossibleMatches::serialize() const -> std::string {
    return fmt::format(
            "Types: {}; Node IDs: {{}}",
            std::bitset<sizeof(m_possible_types)>(m_possible_types).to_string(),
            fmt::join(m_possible_node_ids, ",")
    );
}

auto get_schema_tree_node_queries(SchemaTree const& schema_tree)
        -> std::map<std::string, PossibleMatches> {
    std::map<std::string, PossibleMatches> query_to_possible_matches_map;
    std::vector<std::pair<std::string, SchemaTree::Node::id_t>> search_stack;

    auto push_all_children_to_stack
            = [&](SchemaTree::Node const& node, std::string const& query) -> void {
        for (auto const node_id : node.get_children_ids()) {
            search_stack.emplace_back(query, node_id);
        }
    };

    push_all_children_to_stack(schema_tree.get_root(), "");
    push_all_children_to_stack(schema_tree.get_root(), "*");

    while (false == search_stack.empty()) {
        auto const [parent_query, node_id] = search_stack.back();
        search_stack.pop_back();

        auto const& node{schema_tree.get_node(node_id)};
        bool const is_parent_query_empty{parent_query.empty()};
        bool const parent_query_end_with_wildcard{
                is_parent_query_empty ? false : parent_query.back() == '*'
        };

        if (parent_query_end_with_wildcard) {
            // Handle the trailing wildcard from the parent node
            auto [it, inserted]
                    = query_to_possible_matches_map.try_emplace(parent_query, PossibleMatches{});
            it->second.set_possible_node(node_id, node.get_type());
            push_all_children_to_stack(node, parent_query);
        } else {
            // Otherwise, insert a wildcard
            auto const query_with_wildcard{parent_query + (is_parent_query_empty ? "*" : ".*")};
            auto [it, inserted] = query_to_possible_matches_map.try_emplace(
                    query_with_wildcard,
                    PossibleMatches{}
            );
            it->second.set_possible_node(node_id, node.get_type());
            push_all_children_to_stack(node, query_with_wildcard);
        }

        auto const query_with_curr_key{
                parent_query + (is_parent_query_empty ? "" : ".") + std::string{node.get_key_name()}
        };
        auto [it, inserted]
                = query_to_possible_matches_map.try_emplace(query_with_curr_key, PossibleMatches{});
        it->second.set_possible_node(node_id, node.get_type());
        push_all_children_to_stack(node, query_with_curr_key);
    }

    return query_to_possible_matches_map;
}

auto operator<<(
        std::ostream& os,
        std::map<std::string, PossibleMatches> const& query_to_possible_matches_map
) -> std::ostream& {
    for (auto const& [query, possible_matches] : query_to_possible_matches_map) {
        os << fmt::format("Query: {}; {}\n", query, possible_matches.serialize());
    }
    return os;
}
}  // namespace clp::ffi::ir_stream::search::test
