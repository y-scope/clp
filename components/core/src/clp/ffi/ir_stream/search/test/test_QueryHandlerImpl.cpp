#include <catch2/catch.hpp>

#include "utils.hpp"

namespace clp::ffi::ir_stream::search::test {
TEST_CASE("WIP", "[ffi]") {
    /*
     * <0:root:Obj>
     *      |
     *      |------------> <1:a:Obj>
     *      |                  |
     *      |--> <2:b:Int>     |--> <3:b:Obj>
     *      |                  |        |
     *      |--> <12:a:Int>    |        |------------> <4:c:Obj>
     *                         |        |                  |
     *                         |        |--> <5:d:Str>     |--> <7:a:UnstructuredArray>
     *                         |        |                  |
     *                         |        |--> <6:d:Bool>    |--> <8:d:Str>
     *                         |        |                  |
     *                         |        |--> <10:e:Obj>    |--> <9:d:Float>
     *                         |                           |
     *                         |--> <13:b:Bool>            |--> <11:f:Obj>
     */
    auto const auto_gen_keys_schema_tree{std::make_shared<SchemaTree>()};
    auto const user_gen_keys_schema_tree{std::make_shared<SchemaTree>()};
    std::vector<SchemaTree::NodeLocator> const locators{
            {SchemaTree::cRootId, "a", SchemaTree::Node::Type::Obj},
            {SchemaTree::cRootId, "b", SchemaTree::Node::Type::Int},
            {1, "b", SchemaTree::Node::Type::Obj},
            {3, "c", SchemaTree::Node::Type::Obj},
            {3, "d", SchemaTree::Node::Type::Str},
            {3, "d", SchemaTree::Node::Type::Bool},
            {4, "a", SchemaTree::Node::Type::UnstructuredArray},
            {4, "d", SchemaTree::Node::Type::Str},
            {4, "d", SchemaTree::Node::Type::Float},
            {3, "e", SchemaTree::Node::Type::Obj},
            {4, "f", SchemaTree::Node::Type::Obj},
            {SchemaTree::cRootId, "a", SchemaTree::Node::Type::Int},
            {1, "b", SchemaTree::Node::Type::Bool}
    };
    for (auto const& locator : locators) {
        REQUIRE_NOTHROW(auto_gen_keys_schema_tree->insert_node(locator));
        REQUIRE_NOTHROW(user_gen_keys_schema_tree->insert_node(locator));
    }

    INFO(get_schema_tree_node_queries(*auto_gen_keys_schema_tree));
}
}  // namespace clp::ffi::ir_stream::search::test
