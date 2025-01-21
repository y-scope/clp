#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>
#include <json/single_include/nlohmann/json.hpp>

#include "../src/clp/ffi/encoding_methods.hpp"
#include "../src/clp/ffi/KeyValuePairLogEvent.hpp"
#include "../src/clp/ffi/SchemaTree.hpp"
#include "../src/clp/ffi/Value.hpp"
#include "../src/clp/ir/EncodedTextAst.hpp"
#include "../src/clp/ir/types.hpp"
#include "../src/clp/time_types.hpp"

using clp::ffi::KeyValuePairLogEvent;
using clp::ffi::SchemaTree;
using clp::ffi::Value;
using clp::ffi::value_bool_t;
using clp::ffi::value_float_t;
using clp::ffi::value_int_t;
using clp::ir::eight_byte_encoded_variable_t;
using clp::ir::EightByteEncodedTextAst;
using clp::ir::four_byte_encoded_variable_t;
using clp::ir::FourByteEncodedTextAst;
using clp::UtcOffset;
using std::string;
using std::vector;

namespace {
constexpr std::string_view cStringToEncode{"uid=0, CPU usage: 99.99%, \"user_name\"=YScope"};

/**
 * Parses and encodes the given string as an instance of `EncodedTextAst`.
 * @tparam encoded_variable_t
 * @param text
 * @return The encoded result.
 */
template <typename encoded_variable_t>
requires(
        (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>
         || std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>)
)
[[nodiscard]] auto get_encoded_text_ast(std::string_view text)
        -> clp::ir::EncodedTextAst<encoded_variable_t>;

/**
 * Tests that `Value::is` returns true for the given type and false for all others.
 * @tparam Type The type to query.
 * @param value The value to test against.
 */
template <typename Type>
auto test_value_is(Value const& value) -> void;

/**
 * Tests `Value::get_immutable_view` either:
 * 1. returns the expected value with the expected type for the given type and value;
 * 2. throws for any other type.
 * @tparam Type The type to query.
 * @param value The value to test against.
 * @param typed_value The typed value to compare with.
 */
template <typename Type>
auto test_value_get_immutable_view(Value const& value, Type const& typed_value) -> void;

/**
 * Generates invalid node-ID value pairs with values that don't match the type of the schema tree
 * node with the given ID.
 * @param schema_tree
 * @param node_id
 * @param invalid_node_id_value_pairs Returns the pairs after insertion.
 */
auto insert_invalid_node_id_value_pairs_with_node_type_errors(
        SchemaTree const& schema_tree,
        SchemaTree::Node::id_t node_id,
        KeyValuePairLogEvent::NodeIdValuePairs& invalid_node_id_value_pairs
) -> void;

/**
 * Asserts that `KeyValuePairLogEvent` creation fails with the expected error code.
 * @param auto_gen_keys_schema_tree
 * @param user_gen_keys_schema_tree
 * @param auto_gen_node_id_value_pairs
 * @param user_gen_node_id_value_pairs
 * @param utc_offset
 * @param expected_error_code
 * @return Whether the assertion succeeded.
 */
[[nodiscard]] auto assert_kv_pair_log_event_creation_failure(
        std::shared_ptr<SchemaTree> auto_gen_keys_schema_tree,
        std::shared_ptr<SchemaTree> user_gen_keys_schema_tree,
        KeyValuePairLogEvent::NodeIdValuePairs auto_gen_node_id_value_pairs,
        KeyValuePairLogEvent::NodeIdValuePairs user_gen_node_id_value_pairs,
        UtcOffset utc_offset,
        std::errc expected_error_code
) -> bool;

template <typename encoded_variable_t>
requires(
        (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>
         || std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>)
)
auto get_encoded_text_ast(std::string_view text) -> clp::ir::EncodedTextAst<encoded_variable_t> {
    string logtype;
    vector<encoded_variable_t> encoded_vars;
    vector<int32_t> dict_var_bounds;
    REQUIRE(clp::ffi::encode_message(text, logtype, encoded_vars, dict_var_bounds));
    REQUIRE(((dict_var_bounds.size() % 2) == 0));

    vector<string> dict_vars;
    for (size_t i{0}; i < dict_var_bounds.size(); i += 2) {
        auto const begin_pos{static_cast<size_t>(dict_var_bounds[i])};
        auto const end_pos{static_cast<size_t>(dict_var_bounds[i + 1])};
        dict_vars.emplace_back(text.cbegin() + begin_pos, text.cbegin() + end_pos);
    }

    return clp::ir::EncodedTextAst<encoded_variable_t>{logtype, dict_vars, encoded_vars};
}

template <typename Type>
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto test_value_is(Value const& value) -> void {
    REQUIRE((std::is_same_v<std::monostate, Type> == value.is_null()));
    REQUIRE((std::is_same_v<value_int_t, Type> == value.is<value_int_t>()));
    REQUIRE((std::is_same_v<value_float_t, Type> == value.is<value_float_t>()));
    REQUIRE((std::is_same_v<value_bool_t, Type> == value.is<value_bool_t>()));
    REQUIRE((std::is_same_v<string, Type> == value.is<string>()));
    REQUIRE((std::is_same_v<EightByteEncodedTextAst, Type> == value.is<EightByteEncodedTextAst>()));
    REQUIRE((std::is_same_v<FourByteEncodedTextAst, Type> == value.is<FourByteEncodedTextAst>()));
}

template <typename Type>
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto test_value_get_immutable_view(Value const& value, Type const& typed_value) -> void {
    if constexpr (std::is_same_v<value_int_t, Type>) {
        REQUIRE((value.get_immutable_view<Type>() == typed_value));
        REQUIRE((std::is_same_v<Type, decltype(value.get_immutable_view<Type>())>));
    } else {
        REQUIRE_THROWS(value.get_immutable_view<value_int_t>());
    }

    if constexpr (std::is_same_v<value_float_t, Type>) {
        REQUIRE((value.get_immutable_view<Type>() == typed_value));
        REQUIRE((std::is_same_v<Type, decltype(value.get_immutable_view<Type>())>));
    } else {
        REQUIRE_THROWS(value.get_immutable_view<value_float_t>());
    }

    if constexpr (std::is_same_v<value_bool_t, Type>) {
        REQUIRE((value.get_immutable_view<Type>() == typed_value));
        REQUIRE((std::is_same_v<Type, decltype(value.get_immutable_view<Type>())>));
    } else {
        REQUIRE_THROWS(value.get_immutable_view<value_bool_t>());
    }

    if constexpr (std::is_same_v<string, Type>) {
        REQUIRE((value.get_immutable_view<Type>() == typed_value));
        REQUIRE((std::is_same_v<std::string_view, decltype(value.get_immutable_view<Type>())>));
    } else {
        REQUIRE_THROWS(value.get_immutable_view<string>());
    }

    if constexpr (std::is_same_v<EightByteEncodedTextAst, Type>) {
        REQUIRE((value.get_immutable_view<Type>() == typed_value));
        REQUIRE((std::is_same_v<
                 EightByteEncodedTextAst const&,
                 decltype(value.get_immutable_view<Type>())>));
    } else {
        REQUIRE_THROWS(value.get_immutable_view<EightByteEncodedTextAst>());
    }

    if constexpr (std::is_same_v<FourByteEncodedTextAst, Type>) {
        REQUIRE((value.get_immutable_view<Type>() == typed_value));
        REQUIRE((std::is_same_v<
                 FourByteEncodedTextAst const&,
                 decltype(value.get_immutable_view<Type>())>));
    } else {
        REQUIRE_THROWS(value.get_immutable_view<FourByteEncodedTextAst>());
    }
}

auto insert_invalid_node_id_value_pairs_with_node_type_errors(
        SchemaTree const& schema_tree,
        SchemaTree::Node::id_t node_id,
        KeyValuePairLogEvent::NodeIdValuePairs& invalid_node_id_value_pairs
) -> void {
    REQUIRE((node_id < schema_tree.get_size()));
    auto const node_type{schema_tree.get_node(node_id).get_type()};
    if (SchemaTree::Node::Type::Int != node_type) {
        invalid_node_id_value_pairs.emplace(node_id, Value{static_cast<value_int_t>(0)});
    }
    if (SchemaTree::Node::Type::Float != node_type) {
        invalid_node_id_value_pairs.emplace(node_id, Value{static_cast<value_float_t>(0.0)});
    }
    if (SchemaTree::Node::Type::Bool != node_type) {
        invalid_node_id_value_pairs.emplace(node_id, Value{static_cast<value_bool_t>(false)});
    }
    if (SchemaTree::Node::Type::Str != node_type) {
        invalid_node_id_value_pairs.emplace(node_id, Value{static_cast<string>("Test")});
        if (SchemaTree::Node::Type::UnstructuredArray != node_type) {
            invalid_node_id_value_pairs.emplace(
                    node_id,
                    Value{get_encoded_text_ast<four_byte_encoded_variable_t>(cStringToEncode)}
            );
            invalid_node_id_value_pairs.emplace(
                    node_id,
                    Value{get_encoded_text_ast<eight_byte_encoded_variable_t>(cStringToEncode)}
            );
        }
    }
    if (SchemaTree::Node::Type::Obj != node_type) {
        invalid_node_id_value_pairs.emplace(node_id, std::nullopt);
        invalid_node_id_value_pairs.emplace(node_id, Value{});
    }
}

auto assert_kv_pair_log_event_creation_failure(
        std::shared_ptr<SchemaTree> auto_gen_keys_schema_tree,
        std::shared_ptr<SchemaTree> user_gen_keys_schema_tree,
        KeyValuePairLogEvent::NodeIdValuePairs auto_gen_node_id_value_pairs,
        KeyValuePairLogEvent::NodeIdValuePairs user_gen_node_id_value_pairs,
        UtcOffset utc_offset,
        std::errc expected_error_code
) -> bool {
    auto const result{KeyValuePairLogEvent::create(
            std::move(auto_gen_keys_schema_tree),
            std::move(user_gen_keys_schema_tree),
            std::move(auto_gen_node_id_value_pairs),
            std::move(user_gen_node_id_value_pairs),
            utc_offset
    )};
    return result.has_error() && result.error() == expected_error_code;
}
}  // namespace

TEST_CASE("ffi_Value_basic", "[ffi][Value]") {
    Value const null_value;
    test_value_is<std::monostate>(null_value);
    test_value_get_immutable_view<std::monostate>(null_value, std::monostate{});

    constexpr value_int_t cIntVal{1000};
    Value const int_value{cIntVal};
    test_value_is<value_int_t>(int_value);
    test_value_get_immutable_view<value_int_t>(int_value, cIntVal);

    constexpr value_float_t cFloatValue{1000.0001};
    Value const float_value{cFloatValue};
    test_value_is<value_float_t>(float_value);
    test_value_get_immutable_view<value_float_t>(float_value, cFloatValue);

    constexpr value_bool_t cBoolVal{false};
    Value const bool_value{cBoolVal};
    test_value_is<value_bool_t>(bool_value);
    test_value_get_immutable_view<value_bool_t>(bool_value, cBoolVal);

    constexpr std::string_view cStringVal{"This is a test string message"};
    Value const string_value{string{cStringVal}};
    test_value_is<string>(string_value);
    test_value_get_immutable_view<string>(string_value, string{cStringVal});

    Value const eight_byte_encoded_text_ast_value{
            get_encoded_text_ast<eight_byte_encoded_variable_t>(cStringToEncode)
    };
    test_value_is<EightByteEncodedTextAst>(eight_byte_encoded_text_ast_value);
    test_value_get_immutable_view<EightByteEncodedTextAst>(
            eight_byte_encoded_text_ast_value,
            get_encoded_text_ast<eight_byte_encoded_variable_t>(cStringToEncode)
    );

    Value const four_byte_encoded_text_ast_value{
            get_encoded_text_ast<four_byte_encoded_variable_t>(cStringToEncode)
    };
    test_value_is<FourByteEncodedTextAst>(four_byte_encoded_text_ast_value);
    test_value_get_immutable_view<FourByteEncodedTextAst>(
            four_byte_encoded_text_ast_value,
            get_encoded_text_ast<four_byte_encoded_variable_t>(cStringToEncode)
    );
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("ffi_KeyValuePairLogEvent_create", "[ffi]") {
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

    REQUIRE((*auto_gen_keys_schema_tree == *user_gen_keys_schema_tree));

    SECTION("Test empty ID-value pairs") {
        auto const result{KeyValuePairLogEvent::create(
                auto_gen_keys_schema_tree,
                user_gen_keys_schema_tree,
                {},
                {},
                UtcOffset{0}
        )};
        REQUIRE_FALSE(result.has_error());
    }

    SECTION("Test schema tree pointers being null") {
        REQUIRE(assert_kv_pair_log_event_creation_failure(
                nullptr,
                user_gen_keys_schema_tree,
                {},
                {},
                UtcOffset{0},
                std::errc::invalid_argument
        ));
        REQUIRE(assert_kv_pair_log_event_creation_failure(
                auto_gen_keys_schema_tree,
                nullptr,
                {},
                {},
                UtcOffset{0},
                std::errc::invalid_argument
        ));
    }

    SECTION("Test mismatched types") {
        KeyValuePairLogEvent::NodeIdValuePairs invalid_node_id_value_pairs;
        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        // Int:
        insert_invalid_node_id_value_pairs_with_node_type_errors(
                *user_gen_keys_schema_tree,
                2,
                invalid_node_id_value_pairs
        );

        // Float:
        insert_invalid_node_id_value_pairs_with_node_type_errors(
                *user_gen_keys_schema_tree,
                9,
                invalid_node_id_value_pairs
        );

        // Bool:
        insert_invalid_node_id_value_pairs_with_node_type_errors(
                *user_gen_keys_schema_tree,
                6,
                invalid_node_id_value_pairs
        );

        // Str:
        insert_invalid_node_id_value_pairs_with_node_type_errors(
                *user_gen_keys_schema_tree,
                5,
                invalid_node_id_value_pairs
        );

        // UnstructuredArray:
        insert_invalid_node_id_value_pairs_with_node_type_errors(
                *user_gen_keys_schema_tree,
                7,
                invalid_node_id_value_pairs
        );

        // Obj:
        insert_invalid_node_id_value_pairs_with_node_type_errors(
                *user_gen_keys_schema_tree,
                3,
                invalid_node_id_value_pairs
        );
        // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

        for (auto const& [node_id, optional_value] : invalid_node_id_value_pairs) {
            KeyValuePairLogEvent::NodeIdValuePairs node_id_value_pair_to_test;
            if (optional_value.has_value()) {
                node_id_value_pair_to_test.emplace(node_id, optional_value.value());
            } else {
                node_id_value_pair_to_test.emplace(node_id, std::nullopt);
            }

            REQUIRE(assert_kv_pair_log_event_creation_failure(
                    auto_gen_keys_schema_tree,
                    user_gen_keys_schema_tree,
                    node_id_value_pair_to_test,
                    {},
                    UtcOffset{0},
                    std::errc::protocol_error
            ));
            REQUIRE(assert_kv_pair_log_event_creation_failure(
                    auto_gen_keys_schema_tree,
                    user_gen_keys_schema_tree,
                    {},
                    node_id_value_pair_to_test,
                    UtcOffset{0},
                    std::errc::protocol_error
            ));
        }
    }

    SECTION("Test valid ID-value pairs") {
        constexpr std::string_view cJsonArrayToEncode{"[\"a\", 1, 0.1, null]"};
        constexpr std::string_view cStaticText{"Test"};
        KeyValuePairLogEvent::NodeIdValuePairs valid_node_id_value_pairs;
        /*
         * The sub schema tree of `node_id_value_pairs`:
         * <0:root:Obj>
         *      |
         *      |------------> <1:a:Obj>
         *      |                  |
         *      |--> <2:b:Int>     |--> <3:b:Obj>
         *                                  |
         *                                  |------------> <4:c:Obj>
         *                                  |                  |
         *                                  |--> <5:d:Str>     |--> <7:a:UnstructuredArray>
         *                                  |                  |
         *                                  |                  |--> <8:d:Str>
         *                                  |                  |
         *                                  |--> <10:e:Obj>    |
         *                                                     |
         *                                                     |--> <11:f:Obj>
         */
        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        valid_node_id_value_pairs.emplace(2, Value{static_cast<value_int_t>(0)});
        valid_node_id_value_pairs.emplace(5, Value{string{cStaticText}});
        valid_node_id_value_pairs.emplace(
                8,
                Value{get_encoded_text_ast<four_byte_encoded_variable_t>(cStringToEncode)}
        );
        valid_node_id_value_pairs.emplace(
                7,
                Value{get_encoded_text_ast<eight_byte_encoded_variable_t>(cJsonArrayToEncode)}
        );
        valid_node_id_value_pairs.emplace(10, Value{});
        valid_node_id_value_pairs.emplace(11, std::nullopt);
        // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        auto const result{KeyValuePairLogEvent::create(
                auto_gen_keys_schema_tree,
                user_gen_keys_schema_tree,
                valid_node_id_value_pairs,
                valid_node_id_value_pairs,
                UtcOffset{0}
        )};
        REQUIRE_FALSE(result.has_error());

        SECTION("Test JSON serialization") {
            nlohmann::json const subtree_rooted_at_node_4
                    = {{"a", nlohmann::json::parse(cJsonArrayToEncode)},
                       {"d", cStringToEncode},
                       {"f", nlohmann::json::object_t()}};
            nlohmann::json const subtree_rooted_at_node_3
                    = {{"c", subtree_rooted_at_node_4}, {"d", cStaticText}, {"e", nullptr}};
            nlohmann::json const expected = {
                    {"a", {{"b", subtree_rooted_at_node_3}}},
                    {"b", 0},
            };

            auto const& kv_pair_log_event{result.value()};
            auto const serialized_json_result{kv_pair_log_event.serialize_to_json()};
            REQUIRE_FALSE(serialized_json_result.has_error());
            auto const& [serialized_auto_gen_kv_pairs, serialized_user_gen_kv_pairs]{
                    serialized_json_result.value()
            };
            REQUIRE((serialized_auto_gen_kv_pairs == expected));
            REQUIRE((serialized_user_gen_kv_pairs == expected));
        }

        SECTION("Test duplicated key conflict under node #3") {
            auto invalid_node_id_value_pairs{valid_node_id_value_pairs};
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
            invalid_node_id_value_pairs.emplace(6, Value{static_cast<value_bool_t>(false)});
            REQUIRE(assert_kv_pair_log_event_creation_failure(
                    auto_gen_keys_schema_tree,
                    user_gen_keys_schema_tree,
                    invalid_node_id_value_pairs,
                    valid_node_id_value_pairs,
                    UtcOffset{0},
                    std::errc::protocol_not_supported
            ));
            REQUIRE(assert_kv_pair_log_event_creation_failure(
                    auto_gen_keys_schema_tree,
                    user_gen_keys_schema_tree,
                    valid_node_id_value_pairs,
                    invalid_node_id_value_pairs,
                    UtcOffset{0},
                    std::errc::protocol_not_supported
            ));
        }

        SECTION("Test duplicated key conflict under node #4") {
            auto invalid_node_id_value_pairs{valid_node_id_value_pairs};
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
            invalid_node_id_value_pairs.emplace(9, Value{static_cast<value_float_t>(0.0)});
            REQUIRE(assert_kv_pair_log_event_creation_failure(
                    auto_gen_keys_schema_tree,
                    user_gen_keys_schema_tree,
                    invalid_node_id_value_pairs,
                    valid_node_id_value_pairs,
                    UtcOffset{0},
                    std::errc::protocol_not_supported
            ));
            REQUIRE(assert_kv_pair_log_event_creation_failure(
                    auto_gen_keys_schema_tree,
                    user_gen_keys_schema_tree,
                    valid_node_id_value_pairs,
                    invalid_node_id_value_pairs,
                    UtcOffset{0},
                    std::errc::protocol_not_supported
            ));
        }

        SECTION("Test duplicated keys among siblings of node #1") {
            auto invalid_node_id_value_pairs{valid_node_id_value_pairs};
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
            invalid_node_id_value_pairs.emplace(12, static_cast<value_int_t>(0));
            // Node #12 has the same key as its sibling node #1
            REQUIRE(assert_kv_pair_log_event_creation_failure(
                    auto_gen_keys_schema_tree,
                    user_gen_keys_schema_tree,
                    invalid_node_id_value_pairs,
                    valid_node_id_value_pairs,
                    UtcOffset{0},
                    std::errc::protocol_not_supported
            ));
            REQUIRE(assert_kv_pair_log_event_creation_failure(
                    auto_gen_keys_schema_tree,
                    user_gen_keys_schema_tree,
                    valid_node_id_value_pairs,
                    invalid_node_id_value_pairs,
                    UtcOffset{0},
                    std::errc::protocol_not_supported
            ));
        }

        SECTION("Test duplicated keys among siblings of node #3") {
            auto invalid_node_id_value_pairs{valid_node_id_value_pairs};
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
            invalid_node_id_value_pairs.emplace(13, false);
            // Node #13 has the same key as its sibling node #3
            REQUIRE(assert_kv_pair_log_event_creation_failure(
                    auto_gen_keys_schema_tree,
                    user_gen_keys_schema_tree,
                    invalid_node_id_value_pairs,
                    valid_node_id_value_pairs,
                    UtcOffset{0},
                    std::errc::protocol_not_supported
            ));
            REQUIRE(assert_kv_pair_log_event_creation_failure(
                    auto_gen_keys_schema_tree,
                    user_gen_keys_schema_tree,
                    valid_node_id_value_pairs,
                    invalid_node_id_value_pairs,
                    UtcOffset{0},
                    std::errc::protocol_not_supported
            ));
        }

        SECTION("Test invalid sub-tree on node #3") {
            auto invalid_node_id_value_pairs{valid_node_id_value_pairs};
            invalid_node_id_value_pairs.emplace(3, std::nullopt);
            // Node #3 is empty, but its descendants appear in the sub schema tree (node #5 & #10)
            REQUIRE(assert_kv_pair_log_event_creation_failure(
                    auto_gen_keys_schema_tree,
                    user_gen_keys_schema_tree,
                    invalid_node_id_value_pairs,
                    valid_node_id_value_pairs,
                    UtcOffset{0},
                    std::errc::operation_not_permitted
            ));
            REQUIRE(assert_kv_pair_log_event_creation_failure(
                    auto_gen_keys_schema_tree,
                    user_gen_keys_schema_tree,
                    valid_node_id_value_pairs,
                    invalid_node_id_value_pairs,
                    UtcOffset{0},
                    std::errc::operation_not_permitted
            ));
        }

        SECTION("Test invalid sub-tree on node #4") {
            auto invalid_node_id_value_pairs{valid_node_id_value_pairs};
            invalid_node_id_value_pairs.emplace(4, Value{});
            // Node #4 is null, but its descendants appear in the sub schema tree (node #5 & #10)
            REQUIRE(assert_kv_pair_log_event_creation_failure(
                    auto_gen_keys_schema_tree,
                    user_gen_keys_schema_tree,
                    invalid_node_id_value_pairs,
                    valid_node_id_value_pairs,
                    UtcOffset{0},
                    std::errc::operation_not_permitted
            ));
            REQUIRE(assert_kv_pair_log_event_creation_failure(
                    auto_gen_keys_schema_tree,
                    user_gen_keys_schema_tree,
                    valid_node_id_value_pairs,
                    invalid_node_id_value_pairs,
                    UtcOffset{0},
                    std::errc::operation_not_permitted
            ));
        }
    }

    SECTION("Test out-of-bound node ID") {
        KeyValuePairLogEvent::NodeIdValuePairs node_id_value_pairs_out_of_bound;
        node_id_value_pairs_out_of_bound.emplace(
                static_cast<SchemaTree::Node::id_t>(user_gen_keys_schema_tree->get_size()),
                Value{}
        );
        REQUIRE(assert_kv_pair_log_event_creation_failure(
                auto_gen_keys_schema_tree,
                user_gen_keys_schema_tree,
                node_id_value_pairs_out_of_bound,
                {},
                UtcOffset{0},
                std::errc::operation_not_permitted
        ));
        REQUIRE(assert_kv_pair_log_event_creation_failure(
                auto_gen_keys_schema_tree,
                user_gen_keys_schema_tree,
                {},
                node_id_value_pairs_out_of_bound,
                UtcOffset{0},
                std::errc::operation_not_permitted
        ));
    }
}
