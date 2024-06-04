#include <optional>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>
#include <msgpack.hpp>

#include "../src/clp/ffi/SchemaTree.hpp"
#include "../src/clp/ffi/SchemaTreeNode.hpp"

using clp::ffi::SchemaTree;
using clp::ffi::SchemaTreeNode;

namespace {
/**
 * @param schema_tree
 * @param locator
 * @param expected_id
 * @return Whether the node was inserted successfully with the expected ID.
 */
[[nodiscard]] auto insert_node(
        SchemaTree& schema_tree,
        SchemaTree::NodeLocator locator,
        SchemaTreeNode::id_t expected_id
) -> bool;

/**
 * @param schema_tree
 * @param locator
 * @param expected_id
 * @return Whether the node exists and its ID matches the expected ID.
 */
[[nodiscard]] auto check_node(
        SchemaTree const& schema_tree,
        SchemaTree::NodeLocator locator,
        SchemaTreeNode::id_t expected_id
) -> bool;

auto insert_node(
        SchemaTree& schema_tree,
        SchemaTree::NodeLocator locator,
        SchemaTreeNode::id_t expected_id
) -> bool {
    return false == schema_tree.has_node(locator)
           && expected_id == schema_tree.insert_node(locator);
}

auto check_node(
        SchemaTree const& schema_tree,
        SchemaTree::NodeLocator locator,
        SchemaTreeNode::id_t expected_id
) -> bool {
    auto const node_id{schema_tree.try_get_node_id(locator)};
    return node_id.has_value() && node_id.value() == expected_id;
}
}  // namespace

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
    std::vector<SchemaTree::NodeLocator> const locators{
            {SchemaTree::cRootId, "a", SchemaTreeNode::Type::Obj},
            {SchemaTree::cRootId, "a", SchemaTreeNode::Type::Int},
            {1, "b", SchemaTreeNode::Type::Obj},
            {3, "c", SchemaTreeNode::Type::Obj},
            {3, "d", SchemaTreeNode::Type::Int},
            {3, "d", SchemaTreeNode::Type::Bool},
            {4, "d", SchemaTreeNode::Type::UnstructuredArray},
            {4, "d", SchemaTreeNode::Type::Str}
    };

    auto const snapshot_idx{static_cast<SchemaTreeNode::id_t>(locators.size() / 2)};

    for (SchemaTreeNode::id_t id_to_insert{1}; id_to_insert <= locators.size(); ++id_to_insert) {
        REQUIRE(insert_node(schema_tree, locators[id_to_insert - 1], id_to_insert));
        if (snapshot_idx == id_to_insert) {
            schema_tree.take_snapshot();
        }
    }

    for (SchemaTreeNode::id_t id_to_check{1}; id_to_check <= locators.size(); ++id_to_check) {
        REQUIRE(check_node(schema_tree, locators[id_to_check - 1], id_to_check));
    }

    schema_tree.revert();

    for (SchemaTreeNode::id_t id_to_insert{snapshot_idx + 1}; id_to_insert <= locators.size();
         ++id_to_insert)
    {
        REQUIRE(insert_node(schema_tree, locators[id_to_insert - 1], id_to_insert));
    }

    for (SchemaTreeNode::id_t id_to_check{1}; id_to_check <= locators.size(); ++id_to_check) {
        REQUIRE(check_node(schema_tree, locators[id_to_check - 1], id_to_check));
    }
}
