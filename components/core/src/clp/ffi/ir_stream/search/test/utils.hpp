#ifndef CLP_FFI_IR_STREAM_SEARCH_TEST_UTILS_HPP
#define CLP_FFI_IR_STREAM_SEARCH_TEST_UTILS_HPP

#include <map>
#include <ostream>
#include <set>
#include <string>
#include <tuple>

#include "../../../../../clp_s/search/ast/Literal.hpp"
#include "../../../SchemaTree.hpp"
#include "../utils.hpp"

namespace clp::ffi::ir_stream::search::test {
class PossibleMatches {
public:
    [[nodiscard]] auto get_possible_types() const -> clp_s::search::ast::LiteralTypeBitmask {
        return m_possible_types;
    }

    [[nodiscard]] auto get_possible_ids() const -> std::set<SchemaTree::Node::id_t> const& {
        return m_possible_node_ids;
    }

    auto set_possible_node(SchemaTree::Node::id_t node_id, SchemaTree::Node::Type type) -> void {
        m_possible_node_ids.emplace(node_id);
        m_possible_types |= schema_tree_node_type_to_literal_types(type);
    }

    [[nodiscard]] auto serialize() const -> std::string;

private:
    clp_s::search::ast::LiteralTypeBitmask m_possible_types;
    std::set<SchemaTree::Node::id_t> m_possible_node_ids;
};

/**
 * Gets all possible queries to every single node in the schema tree with a bitmask indicating all
 * the potentially matched types.
 * NOTE: It is assume that all the keys in the schema tree to test don't contain escaped characters.
 * @param schema_tree
 * @return A query-to-possible-matches map.
 */
[[nodiscard]] auto get_schema_tree_node_queries(SchemaTree const& schema_tree)
        -> std::map<std::string, PossibleMatches>;

[[nodiscard]] auto operator<<(
        std::ostream& os,
        std::map<std::string, PossibleMatches> const& query_to_possible_matches_map
) -> std::ostream&;
}  // namespace clp::ffi::ir_stream::search::test

#endif  // CLP_FFI_IR_STREAM_SEARCH_TEST_UTILS_HPP
