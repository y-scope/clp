#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>
#include <msgpack.hpp>

#include "../src/clp/ffi/SchemaTree.hpp"

using clp::ffi::SchemaTree;

namespace {
/**
 * @param schema_tree
 * @param locator
 * @param expected_id
 * @return Whether the node was inserted successfully with the expected ID.
 */
[[nodiscard]] auto insert_node(
        SchemaTree& schema_tree,
        SchemaTree::NodeLocator const& locator,
        SchemaTree::Node::id_t expected_id
) -> bool;

/**
 * @param schema_tree
 * @param locator
 * @param expected_id
 * @return Whether an ID could be found for a non root node matching the locator, the ID matches the
 * expected ID, the corresponding node is not the root, and it matches the locator.
 */
[[nodiscard]] auto check_non_root_node(
        SchemaTree const& schema_tree,
        SchemaTree::NodeLocator const& locator,
        SchemaTree::Node::id_t expected_id
) -> bool;

auto insert_node(
        SchemaTree& schema_tree,
        SchemaTree::NodeLocator const& locator,
        SchemaTree::Node::id_t expected_id
) -> bool {
    return false == schema_tree.has_node(locator)
           && expected_id == schema_tree.insert_node(locator);
}

auto check_non_root_node(
        SchemaTree const& schema_tree,
        SchemaTree::NodeLocator const& locator,
        SchemaTree::Node::id_t expected_id
) -> bool {
    auto const node_id{schema_tree.try_get_node_id(locator)};
    if (false == node_id.has_value() || node_id.value() != expected_id) {
        // The node's ID doesn't match.
        return false;
    }
    auto const& node{schema_tree.get_node(expected_id)};
    if (node.is_root()) {
        // Any nodes added after the tree was constructed must not be the root.
        return false;
    }
    auto const optional_parent_id{node.get_parent_id()};
    if (false == optional_parent_id.has_value()) {
        // Non-root nodes must have a parent ID.
        return false;
    }
    if (optional_parent_id.value() != locator.get_parent_id()
        || node.get_type() != locator.get_type() || node.get_key_name() != locator.get_key_name())
    {
        // The node information doesn't match the locator.
        return false;
    }
    return true;
}
}  // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("ffi_schema_tree", "[ffi]") {
    /*
     * <0:root:Obj>
     *      |
     *      |------------> <1:a:Obj>
     *      |                  |
     *      |--> <2:a:Int>     |--> <3:b:Obj>
     *                                  |
     *                                  |------------> <4:c:Obj>
     *                                  |                  |
     *                                  |--> <5:d:Int>     |--> <7:d:UnstructuredArray>
     *                                  |                  |
     *                                  |--> <6:d:Bool>    |--> <8:d:Str>
     */
    SchemaTree schema_tree;

    // Check the root node
    auto const& root{schema_tree.get_root()};
    REQUIRE((SchemaTree::cRootId == root.get_id()));
    REQUIRE(root.is_root());
    REQUIRE_FALSE(root.get_parent_id().has_value());

    std::vector<SchemaTree::NodeLocator> const locators{
            {SchemaTree::cRootId, "a", SchemaTree::Node::Type::Obj},
            {SchemaTree::cRootId, "a", SchemaTree::Node::Type::Int},
            {1, "b", SchemaTree::Node::Type::Obj},
            {3, "c", SchemaTree::Node::Type::Obj},
            {3, "d", SchemaTree::Node::Type::Int},
            {3, "d", SchemaTree::Node::Type::Bool},
            {4, "d", SchemaTree::Node::Type::UnstructuredArray},
            {4, "d", SchemaTree::Node::Type::Str}
    };

    auto const snapshot_idx{static_cast<SchemaTree::Node::id_t>(locators.size() / 2)};

    for (SchemaTree::Node::id_t id_to_insert{1}; id_to_insert <= locators.size(); ++id_to_insert) {
        REQUIRE(insert_node(schema_tree, locators[id_to_insert - 1], id_to_insert));
        if (snapshot_idx == id_to_insert) {
            schema_tree.take_snapshot();
        }
    }

    for (SchemaTree::Node::id_t id_to_check{1}; id_to_check <= locators.size(); ++id_to_check) {
        REQUIRE(check_non_root_node(schema_tree, locators[id_to_check - 1], id_to_check));
    }

    schema_tree.revert();

    for (SchemaTree::Node::id_t id_to_insert{snapshot_idx + 1}; id_to_insert <= locators.size();
         ++id_to_insert)
    {
        REQUIRE(insert_node(schema_tree, locators[id_to_insert - 1], id_to_insert));
    }

    for (SchemaTree::Node::id_t id_to_check{1}; id_to_check <= locators.size(); ++id_to_check) {
        REQUIRE(check_non_root_node(schema_tree, locators[id_to_check - 1], id_to_check));
    }
}
