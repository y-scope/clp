#include <Catch2/single_include/catch2/catch.hpp>
#include <msgpack.hpp>

#include "../src/clp/ffi/SchemaTree.hpp"
#include "../src/clp/ffi/SchemaTreeNode.hpp"

namespace {
/**
 * @param schema_tree
 * @param locator
 * @param expected_id
 * @return Whether the node is inserted successfully with the expected node id.
 */
[[nodiscard]] auto insert_node(
        clp::ffi::SchemaTree& schema_tree,
        clp::ffi::SchemaTree::TreeNodeLocator locator,
        clp::ffi::SchemaTreeNode::id_t expected_id
) -> bool;

/**
 * @param schema_tree
 * @param locator
 * @param expected_id
 * @return Whether the node exists and its ID matches the expected ID.
 */
[[nodiscard]] auto check_node(
        clp::ffi::SchemaTree const& schema_tree,
        clp::ffi::SchemaTree::TreeNodeLocator locator,
        clp::ffi::SchemaTreeNode::id_t expected_id
) -> bool;

auto insert_node(
        clp::ffi::SchemaTree& schema_tree,
        clp::ffi::SchemaTree::TreeNodeLocator locator,
        clp::ffi::SchemaTreeNode::id_t expected_id
) -> bool {
    clp::ffi::SchemaTreeNode::id_t node_id{};
    return false == schema_tree.try_get_node_id(locator, node_id)
           && expected_id == schema_tree.insert_node(locator);
}

auto check_node(
        clp::ffi::SchemaTree const& schema_tree,
        clp::ffi::SchemaTree::TreeNodeLocator locator,
        clp::ffi::SchemaTreeNode::id_t expected_id
) -> bool {
    clp::ffi::SchemaTreeNode::id_t node_id{};
    return schema_tree.try_get_node_id(locator, node_id) && expected_id == node_id;
}
}  // namespace

TEST_CASE("ffi_schema_tree", "[ffi]") {
    using clp::ffi::SchemaTree;
    using clp::ffi::SchemaTreeNode;

    SchemaTree schema_tree;

    REQUIRE(insert_node(schema_tree, {SchemaTree::cRootId, "a", SchemaTreeNode::Type::Obj}, 1));
    REQUIRE(insert_node(schema_tree, {SchemaTree::cRootId, "a", SchemaTreeNode::Type::Int}, 2));
    REQUIRE(insert_node(schema_tree, {1, "b", SchemaTreeNode::Type::Obj}, 3));
    REQUIRE(insert_node(schema_tree, {3, "c", SchemaTreeNode::Type::Obj}, 4));

    schema_tree.take_snapshot();

    REQUIRE(insert_node(schema_tree, {3, "d", SchemaTreeNode::Type::Int}, 5));
    REQUIRE(insert_node(schema_tree, {3, "d", SchemaTreeNode::Type::Bool}, 6));
    REQUIRE(insert_node(schema_tree, {4, "d", SchemaTreeNode::Type::UnstructuredArray}, 7));
    REQUIRE(insert_node(schema_tree, {4, "d", SchemaTreeNode::Type::Str}, 8));

    REQUIRE(check_node(schema_tree, {SchemaTree::cRootId, "a", SchemaTreeNode::Type::Obj}, 1));
    REQUIRE(check_node(schema_tree, {SchemaTree::cRootId, "a", SchemaTreeNode::Type::Int}, 2));
    REQUIRE(check_node(schema_tree, {1, "b", SchemaTreeNode::Type::Obj}, 3));
    REQUIRE(check_node(schema_tree, {3, "c", SchemaTreeNode::Type::Obj}, 4));
    REQUIRE(check_node(schema_tree, {3, "d", SchemaTreeNode::Type::Int}, 5));
    REQUIRE(check_node(schema_tree, {3, "d", SchemaTreeNode::Type::Bool}, 6));
    REQUIRE(check_node(schema_tree, {4, "d", SchemaTreeNode::Type::UnstructuredArray}, 7));
    REQUIRE(check_node(schema_tree, {4, "d", SchemaTreeNode::Type::Str}, 8));

    schema_tree.revert();

    REQUIRE(check_node(schema_tree, {SchemaTree::cRootId, "a", SchemaTreeNode::Type::Obj}, 1));
    REQUIRE(check_node(schema_tree, {SchemaTree::cRootId, "a", SchemaTreeNode::Type::Int}, 2));
    REQUIRE(check_node(schema_tree, {1, "b", SchemaTreeNode::Type::Obj}, 3));
    REQUIRE(check_node(schema_tree, {3, "c", SchemaTreeNode::Type::Obj}, 4));

    REQUIRE(insert_node(schema_tree, {3, "d", SchemaTreeNode::Type::Int}, 5));
    REQUIRE(insert_node(schema_tree, {3, "d", SchemaTreeNode::Type::Bool}, 6));
    REQUIRE(insert_node(schema_tree, {4, "d", SchemaTreeNode::Type::UnstructuredArray}, 7));
    REQUIRE(insert_node(schema_tree, {4, "d", SchemaTreeNode::Type::Str}, 8));

    REQUIRE(check_node(schema_tree, {3, "d", SchemaTreeNode::Type::Int}, 5));
    REQUIRE(check_node(schema_tree, {3, "d", SchemaTreeNode::Type::Bool}, 6));
    REQUIRE(check_node(schema_tree, {4, "d", SchemaTreeNode::Type::UnstructuredArray}, 7));
    REQUIRE(check_node(schema_tree, {4, "d", SchemaTreeNode::Type::Str}, 8));
}
