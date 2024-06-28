#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>
#include <json/single_include/nlohmann/json.hpp>
#include <msgpack.hpp>

#include "../src/clp/ffi/utils.hpp"
#include "../src/clp/type_utils.hpp"

using nlohmann::json;
using std::optional;
using std::string;
using std::string_view;
using std::vector;

using clp::ffi::serialize_and_append_msgpack_array_to_json_str;
using clp::ffi::serialize_and_append_msgpack_map_to_json_str;

namespace {
/**
 * Serializes the given msgpack byte sequence into a JSON string.
 * @param msgpack_bytes
 * @return Serialized JSON string on success.
 * @return std::nullopt on failure.
 */
[[nodiscard]] auto serialize_msgpack_bytes_to_json_str(vector<unsigned char> const& msgpack_bytes
) -> optional<string>;

auto serialize_msgpack_bytes_to_json_str(vector<unsigned char> const& msgpack_bytes
) -> optional<string> {
    msgpack::object_handle msgpack_oh;
    msgpack::unpack(
            msgpack_oh,
            clp::size_checked_pointer_cast<char const>(msgpack_bytes.data()),
            msgpack_bytes.size()
    );
    optional<string> ret_val;
    auto const& msgpack_obj{msgpack_oh.get()};
    if (msgpack::type::ARRAY == msgpack_obj.type) {
        if (false == serialize_and_append_msgpack_array_to_json_str(msgpack_obj, ret_val.emplace()))
        {
            return std::nullopt;
        }
    } else if (msgpack::type::MAP == msgpack_obj.type) {
        if (false == serialize_and_append_msgpack_map_to_json_str(msgpack_obj, ret_val.emplace())) {
            return std::nullopt;
        }
    } else {
        return std::nullopt;
    }
    return ret_val;
}
}  // namespace

TEST_CASE("test_msgpack_to_json", "[ffi][utils]") {
    optional<string> result;

    // Test array with primitive values only
    json const array_with_primitive_values_only
            = {1, -1, 1.01, -1.01, true, false, "short_string", "This is a long string", nullptr};
    result = serialize_msgpack_bytes_to_json_str(json::to_msgpack(array_with_primitive_values_only)
    );
    REQUIRE((result.has_value() && array_with_primitive_values_only == json::parse(result.value()))
    );

    // Test map with primitive values only
    json const map_with_primitive_values_only
            = {{"int_key", 1},
               {"int_key_negative", -1},
               {"float_key", 0.01},
               {"float_key_negative", -0.01},
               {"bool_key_true", false},
               {"bool_key_false", true},
               {"str_key", "Test string"},
               {"null_key", nullptr}};
    result = serialize_msgpack_bytes_to_json_str(json::to_msgpack(map_with_primitive_values_only));
    REQUIRE((result.has_value() && map_with_primitive_values_only == json::parse(result.value())));

    // Test array with inner map
    json array_with_map = array_with_primitive_values_only;
    array_with_map.emplace_back(map_with_primitive_values_only);
    result = serialize_msgpack_bytes_to_json_str(json::to_msgpack(array_with_map));
    REQUIRE((result.has_value() && array_with_map == json::parse(result.value())));

    // Test map with inner array
    json map_with_array = map_with_primitive_values_only;
    map_with_array.emplace("array_key", array_with_primitive_values_only);
    result = serialize_msgpack_bytes_to_json_str(json::to_msgpack(map_with_array));
    REQUIRE((result.has_value() && map_with_array == json::parse(result.value())));

    // Recursively create inner maps and arrays
    // Note: the execution time and memory consumption will grow exponentially as we increase the
    // recursive depth.
    constexpr size_t cRecursiveDepth{6};
    for (size_t i{0}; i < cRecursiveDepth; ++i) {
        array_with_map.emplace_back(map_with_array);
        array_with_map.emplace_back(array_with_map);
        result = serialize_msgpack_bytes_to_json_str(json::to_msgpack(array_with_map));
        REQUIRE((result.has_value() && array_with_map == json::parse(result.value())));

        map_with_array.emplace("array_key_" + std::to_string(i), array_with_map);
        map_with_array.emplace("map_key_" + std::to_string(i), map_with_array);
        result = serialize_msgpack_bytes_to_json_str(json::to_msgpack(map_with_array));
        REQUIRE((result.has_value() && map_with_array == json::parse(result.value())));
    }
}
