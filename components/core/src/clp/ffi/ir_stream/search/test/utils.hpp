#ifndef CLP_FFI_IR_STREAM_SEARCH_TEST_UTILS_HPP
#define CLP_FFI_IR_STREAM_SEARCH_TEST_UTILS_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch.hpp>
#include <msgpack.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include "../../../../../clp_s/search/ast/Literal.hpp"
#include "../../../../ir/EncodedTextAst.hpp"
#include "../../../../ir/types.hpp"
#include "../../../../type_utils.hpp"
#include "../../../encoding_methods.hpp"
#include "../../../SchemaTree.hpp"
#include "../../Serializer.hpp"
#include "../utils.hpp"

namespace clp::ffi::ir_stream::search::test {
/**
 * Represent all the matchable clp-s literal types and the matchable schema-tree node IDs of a
 * column query.
 */
class ColumnQueryPossibleMatches {
public:
    explicit ColumnQueryPossibleMatches(std::shared_ptr<SchemaTree> schema_tree)
            : m_schema_tree{std::move(schema_tree)} {}

    [[nodiscard]] auto get_matchable_types() const -> clp_s::search::ast::literal_type_bitmask_t {
        return m_matchable_types;
    }

    [[nodiscard]] auto get_matchable_node_ids() const -> std::set<SchemaTree::Node::id_t> const& {
        return m_matchable_node_ids;
    }

    [[nodiscard]] auto get_matchable_node_ids_from_literal_type(
            clp_s::search::ast::LiteralType type
    ) const -> std::vector<SchemaTree::Node::id_t>;

    [[nodiscard]] auto get_matchable_node_ids_from_schema_tree_type(
            SchemaTree::Node::Type type
    ) const -> std::vector<SchemaTree::Node::id_t>;

    auto set_matchable_node(SchemaTree::Node::id_t node_id, SchemaTree::Node::Type type) -> void {
        m_matchable_node_ids.emplace(node_id);
        m_matchable_types |= schema_tree_node_type_to_literal_types(type);
    }

    /**
     * Serializes the underlying matchable types and matchable node IDs in human-readable form for
     * debugging purposes.
     * @return The serialized possible matches.
     */
    [[nodiscard]] auto serialize() const -> std::string;

private:
    std::shared_ptr<SchemaTree> m_schema_tree;
    clp_s::search::ast::literal_type_bitmask_t m_matchable_types{};
    std::set<SchemaTree::Node::id_t> m_matchable_node_ids;
};

/**
 * Trivial implementation of `NewProjectedSchemaTreeNodeCallback` that always return success without
 * doing anything.
 * @param is_auto_generated
 * @param node_id
 * @param projected_key_path
 * @return A void result.
 */
[[nodiscard]] auto trivial_new_projected_schema_tree_node_callback(
        bool is_auto_generated,
        SchemaTree::Node::id_t node_id,
        std::string_view projected_key_path
) -> ystdlib::error_handling::Result<void>;

/**
 * Gets all possible column queries to every single node in the schema-tree with a bitmask
 * indicating all the potentially matched types.
 * NOTE: It is assumed that all the keys in the schema-tree to test don't contain escaped chars.
 * @param schema_tree
 * @return A column-query-to-possible-matches map.
 */
[[nodiscard]] auto get_schema_tree_column_queries(std::shared_ptr<SchemaTree> const& schema_tree)
        -> std::map<std::string, ColumnQueryPossibleMatches>;

[[maybe_unused]] auto operator<<(
        std::ostream& os,
        std::map<std::string, ColumnQueryPossibleMatches> const& column_query_to_possible_matches
) -> std::ostream&;

/**
 * Parses and encodes the given string as an instance of `EncodedTextAst`.
 * @tparam encoded_variable_t
 * @param text
 * @return The encoded result.
 */
template <typename encoded_variable_t>
requires std::is_same_v<encoded_variable_t, ir::eight_byte_encoded_variable_t>
         || std::is_same_v<encoded_variable_t, ir::four_byte_encoded_variable_t>
[[nodiscard]] auto get_encoded_text_ast(std::string_view text)
        -> clp::ir::EncodedTextAst<encoded_variable_t>;

/**
 * Unpacks and serializes the given msgpack bytes using the given serializer.
 * @tparam encoded_variable_t
 * @param auto_gen_msgpack_bytes
 * @param user_gen_msgpack_bytes
 * @param serializer
 * @return Whether serialization succeeded.
 */
template <typename encoded_variable_t>
[[nodiscard]] auto unpack_and_serialize_msgpack_bytes(
        std::vector<uint8_t> const& auto_gen_msgpack_bytes,
        std::vector<uint8_t> const& user_gen_msgpack_bytes,
        Serializer<encoded_variable_t>& serializer
) -> bool;

template <typename encoded_variable_t>
requires std::is_same_v<encoded_variable_t, ir::eight_byte_encoded_variable_t>
         || std::is_same_v<encoded_variable_t, ir::four_byte_encoded_variable_t>
auto get_encoded_text_ast(std::string_view text) -> clp::ir::EncodedTextAst<encoded_variable_t> {
    std::string logtype;
    std::vector<encoded_variable_t> encoded_vars;
    std::vector<int32_t> dict_var_bounds;
    REQUIRE(clp::ffi::encode_message(text, logtype, encoded_vars, dict_var_bounds));
    REQUIRE(((dict_var_bounds.size() % 2) == 0));

    std::vector<std::string> dict_vars;
    for (size_t i{0}; i < dict_var_bounds.size(); i += 2) {
        auto const begin_pos{static_cast<size_t>(dict_var_bounds[i])};
        auto const end_pos{static_cast<size_t>(dict_var_bounds[i + 1])};
        dict_vars.emplace_back(text.cbegin() + begin_pos, text.cbegin() + end_pos);
    }

    return clp::ir::EncodedTextAst<encoded_variable_t>{logtype, dict_vars, encoded_vars};
}

template <typename encoded_variable_t>
auto unpack_and_serialize_msgpack_bytes(
        std::vector<uint8_t> const& auto_gen_msgpack_bytes,
        std::vector<uint8_t> const& user_gen_msgpack_bytes,
        Serializer<encoded_variable_t>& serializer
) -> bool {
    // NOLINTNEXTLINE(misc-include-cleaner)
    auto const auto_gen_msgpack_byte_handle{msgpack::unpack(
            clp::size_checked_pointer_cast<char const>(auto_gen_msgpack_bytes.data()),
            auto_gen_msgpack_bytes.size()
    )};
    auto const auto_gen_msgpack_obj{auto_gen_msgpack_byte_handle.get()};

    // NOLINTNEXTLINE(misc-include-cleaner)
    if (msgpack::type::MAP != auto_gen_msgpack_obj.type) {
        return false;
    }

    // NOLINTNEXTLINE(misc-include-cleaner)
    auto const user_gen_msgpack_byte_handle{msgpack::unpack(
            clp::size_checked_pointer_cast<char const>(user_gen_msgpack_bytes.data()),
            user_gen_msgpack_bytes.size()
    )};
    auto const user_gen_msgpack_obj{user_gen_msgpack_byte_handle.get()};

    // NOLINTNEXTLINE(misc-include-cleaner)
    if (msgpack::type::MAP != user_gen_msgpack_obj.type) {
        return false;
    }

    // The following clang-tidy suppression is needed because it's the only way to access the
    // msgpack object as a map.
    // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
    return serializer.serialize_msgpack_map(
            auto_gen_msgpack_obj.via.map,
            user_gen_msgpack_obj.via.map
    );
    // NOLINTEND(cppcoreguidelines-pro-type-union-access)
}
}  // namespace clp::ffi::ir_stream::search::test

#endif  // CLP_FFI_IR_STREAM_SEARCH_TEST_UTILS_HPP
