#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_message.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <fmt/format.h>
#include <nlohmann/json_fwd.hpp>

#include "../../../../../clp_s/search/kql/kql.hpp"
#include "../../../../BufferReader.hpp"
#include "../../../../ir/types.hpp"
#include "../../../../time_types.hpp"
#include "../../../../type_utils.hpp"
#include "../../../KeyValuePairLogEvent.hpp"
#include "../../../SchemaTree.hpp"
#include "../../Deserializer.hpp"
#include "../../IrUnitType.hpp"
#include "../../protocol_constants.hpp"
#include "../../Serializer.hpp"
#include "../QueryHandler.hpp"
#include "utils.hpp"

// This include has a circular dependency with the `.inc` file.
// The following clang-tidy suppression should be removed once the circular dependency is resolved.
// NOLINTNEXTLINE(misc-header-include-cycle)
#include "../../decoding_methods.hpp"

namespace clp::ffi::ir_stream::search::test {
namespace {
using JsonPair = std::pair<nlohmann::json, nlohmann::json>;

/**
 * Implementation of `clp::ffi::ir_stream::IrUnitHandlerReq` for testing purposes.
 */
class IrUnitHandler {
public:
    [[nodiscard]] auto
    handle_log_event(KeyValuePairLogEvent&& log_event, [[maybe_unused]] size_t log_event_idx)
            -> IRErrorCode {
        m_deserialized_log_events.emplace_back(std::move(log_event));
        return IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] static auto handle_utc_offset_change(
            [[maybe_unused]] UtcOffset utc_offset_old,
            [[maybe_unused]] UtcOffset utc_offset_new
    ) -> IRErrorCode {
        return IRErrorCode::IRErrorCode_Decode_Error;
    }

    [[nodiscard]] static auto handle_schema_tree_node_insertion(
            [[maybe_unused]] bool is_auto_generated,
            [[maybe_unused]] clp::ffi::SchemaTree::NodeLocator schema_tree_node_locator,
            [[maybe_unused]] std::shared_ptr<clp::ffi::SchemaTree const> const& schema_tree
    ) -> IRErrorCode {
        return IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] auto handle_end_of_stream() -> IRErrorCode {
        m_is_complete = true;
        return IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] auto get_deserialized_log_events() const
            -> std::vector<KeyValuePairLogEvent> const& {
        return m_deserialized_log_events;
    }

    [[nodiscard]] auto is_complete() const -> bool { return m_is_complete; }

private:
    std::vector<KeyValuePairLogEvent> m_deserialized_log_events;
    bool m_is_complete{false};
};

/**
 * Serializes a vector of JSON object pairs into a key-value pair IR stream, where each pair is
 * treated as containing an object with auto-generated KV pairs and an object with user-generated KV
 * pairs.
 * @tparam encoded_variable_t
 * @param json_pairs
 * @return A vector of bytes representing the serialized IR stream.
 */
template <typename encoded_variable_t>
[[nodiscard]] auto serialize_json_pairs_into_kv_pair_ir_stream(
        std::vector<JsonPair> const& json_pairs
) -> std::vector<int8_t>;

/**
 * Serializes a vector of JSON object pairs into a string, where each pair represents the
 * auto-generated and user-generated KV pairs in a log event.
 * @param json_pairs
 * @return The string.
 */
[[nodiscard]] auto serialize_json_pairs_to_str(std::vector<JsonPair> const& json_pairs)
        -> std::string;

template <typename encoded_variable_t>
auto serialize_json_pairs_into_kv_pair_ir_stream(std::vector<JsonPair> const& json_pairs)
        -> std::vector<int8_t> {
    auto serializer_result{Serializer<encoded_variable_t>::create()};
    REQUIRE_FALSE(serializer_result.has_error());
    auto& serializer{serializer_result.value()};
    for (auto const& [auto_gen_kv_pairs, user_gen_kv_pairs] : json_pairs) {
        REQUIRE_FALSE(unpack_and_serialize_msgpack_bytes(
                              nlohmann::json::to_msgpack(auto_gen_kv_pairs),
                              nlohmann::json::to_msgpack(user_gen_kv_pairs),
                              serializer
        )
                              .has_error());
    }
    auto const ir_buf_view{serializer.get_ir_buf_view()};
    std::vector<int8_t> ir_buf{ir_buf_view.begin(), ir_buf_view.end()};
    ir_buf.emplace_back(static_cast<int8_t>(cProtocol::Eof));
    return ir_buf;
}

auto serialize_json_pairs_to_str(std::vector<JsonPair> const& json_pairs) -> std::string {
    std::string serialized_json_pairs;
    for (auto const& [auto_gen, user_gen] : json_pairs) {
        serialized_json_pairs += fmt::format(
                "auto-gen-kv-pairs:{} | user-gen-kv-pairs:{}\n",
                auto_gen.dump(),
                user_gen.dump()
        );
    }
    return serialized_json_pairs;
}
}  // namespace

TEMPLATE_TEST_CASE(
        "deserialization_kv_ir_stream_with_query",
        "[ffi][ir_stream][search][QueryHandler]",
        ir::four_byte_encoded_variable_t,
        ir::eight_byte_encoded_variable_t
) {
    constexpr std::string_view cIntKey{"int_key"};
    constexpr std::string_view cStrKey{"str_key"};
    constexpr std::string_view cObjKey{"obj_key"};
    constexpr std::string_view cUnresolvableKey{"unresolvable_key"};
    constexpr int cIntBase{INT16_MAX};
    constexpr std::string_view cTestSubStr{"test"};

    nlohmann::json const base_obj_0
            = {{cIntKey, cIntBase - 1}, {cStrKey, fmt::format("VarStr:{}", cTestSubStr)}};
    nlohmann::json const base_obj_1 = {
            {cIntKey, cIntBase + 1},
            {cStrKey,
             fmt::format("This is a {} ClpStr; And it contains a var={}", cTestSubStr, cIntBase)}
    };
    nlohmann::json const nested_obj_0 = {{cObjKey, base_obj_0}};
    nlohmann::json const nested_obj_1 = {{cObjKey, base_obj_1}};

    JsonPair const json_pair_0{base_obj_0, base_obj_1};
    JsonPair const json_pair_1{base_obj_1, base_obj_0};
    JsonPair const json_pair_2{nested_obj_0, nested_obj_1};
    JsonPair const json_pair_3{nested_obj_1, nested_obj_0};

    std::vector<JsonPair> const
            json_pairs_to_serialize{json_pair_0, json_pair_1, json_pair_2, json_pair_3};

    auto const ir_stream_bytes{
            serialize_json_pairs_into_kv_pair_ir_stream<TestType>(json_pairs_to_serialize)
    };
    CAPTURE(ir_stream_bytes.size());

    auto const wildcard_str_query{fmt::format("*{}*", cTestSubStr)};
    auto const [kql_query_str, expected_json_pairs] = GENERATE_COPY(
            std::make_pair(
                    fmt::format("{}: {}", cStrKey, wildcard_str_query),
                    std::vector<JsonPair>{json_pair_0, json_pair_1}
            ),
            std::make_pair(
                    fmt::format("*.{}.*: {}", cStrKey, wildcard_str_query),
                    json_pairs_to_serialize
            ),
            std::make_pair(
                    fmt::format("*.{}.*: {}", cObjKey, wildcard_str_query),
                    std::vector<JsonPair>{json_pair_2, json_pair_3}
            ),
            std::make_pair(
                    fmt::format("NOT *.{}.*: {}", cObjKey, wildcard_str_query),
                    std::vector<JsonPair>{}
            ),
            std::make_pair(
                    fmt::format("{} >= {}", cIntKey, cIntBase),
                    std::vector<JsonPair>{json_pair_0}
            ),
            std::make_pair(
                    fmt::format("*.{} >= {}", cIntKey, cIntBase),
                    std::vector<JsonPair>{
                            json_pair_0,
                            json_pair_2,
                    }
            ),
            std::make_pair(
                    fmt::format("{} <= {}", cIntKey, cIntBase),
                    std::vector<JsonPair>{json_pair_1}
            ),
            std::make_pair(
                    fmt::format("*.{} <= {}", cIntKey, cIntBase),
                    std::vector<JsonPair>{json_pair_1, json_pair_3}
            ),
            std::make_pair(
                    fmt::format("@*.{} <= {} AND *.{} >= {}", cIntKey, cIntBase, cIntKey, cIntBase),
                    std::vector<JsonPair>{json_pair_0, json_pair_2}
            ),
            std::make_pair(
                    fmt::format("@*.{} >= {} AND *.{} <= {}", cIntKey, cIntBase, cIntKey, cIntBase),
                    std::vector<JsonPair>{json_pair_1, json_pair_3}
            ),
            std::make_pair(
                    fmt::format("@*.{} > {} OR *.{} > {}", cIntKey, cIntBase, cIntKey, cIntBase),
                    json_pairs_to_serialize
            ),
            std::make_pair(
                    fmt::format("@*.{} < {} OR *.{} < {}", cIntKey, cIntBase, cIntKey, cIntBase),
                    json_pairs_to_serialize
            ),
            std::make_pair(
                    fmt::format("@*.{} > {} AND *.{} > {}", cIntKey, cIntBase, cIntKey, cIntBase),
                    std::vector<JsonPair>{}
            ),
            std::make_pair(
                    fmt::format(
                            "@*.{} < {} AND NOT *.{} < {}",
                            cIntKey,
                            cIntBase,
                            cIntKey,
                            cIntBase
                    ),
                    std::vector<JsonPair>{json_pair_0, json_pair_2}
            ),
            std::make_pair(
                    fmt::format(
                            "@*.{} > {} AND NOT *.{} > {}",
                            cIntKey,
                            cIntBase,
                            cIntKey,
                            cIntBase
                    ),
                    std::vector<JsonPair>{json_pair_1, json_pair_3}
            ),
            std::make_pair(
                    fmt::format(
                            "NOT (@*.{} > {} AND *.{} > {})",
                            cIntKey,
                            cIntBase,
                            cIntKey,
                            cIntBase
                    ),
                    json_pairs_to_serialize
            ),
            std::make_pair(
                    fmt::format(
                            "NOT (@*.{} < {} AND *.{} < {})",
                            cIntKey,
                            cIntBase,
                            cIntKey,
                            cIntBase
                    ),
                    json_pairs_to_serialize
            ),
            std::make_pair(fmt::format("*.{}.*: *", cUnresolvableKey), std::vector<JsonPair>{}),
            std::make_pair(fmt::format("NOT *.{}.*: *", cUnresolvableKey), std::vector<JsonPair>{})
    );
    CAPTURE(kql_query_str);

    std::istringstream query_stream{kql_query_str};
    auto query{clp_s::search::kql::parse_kql_expression(query_stream)};
    REQUIRE((nullptr != query));

    auto query_handler_result{
            QueryHandler<decltype(&trivial_new_projected_schema_tree_node_callback)>::create(
                    &trivial_new_projected_schema_tree_node_callback,
                    query,
                    {},
                    false
            )
    };
    REQUIRE_FALSE(query_handler_result.has_error());

    BufferReader buf_reader{
            size_checked_pointer_cast<char const>(ir_stream_bytes.data()),
            ir_stream_bytes.size()
    };
    auto deserializer_result{
            make_deserializer(buf_reader, IrUnitHandler{}, std::move(query_handler_result.value()))
    };
    REQUIRE_FALSE(deserializer_result.has_error());
    auto& deserializer{deserializer_result.value()};
    while (true) {
        auto const result{deserializer.deserialize_next_ir_unit(buf_reader)};
        REQUIRE_FALSE(result.has_error());
        if (result.value() == IrUnitType::EndOfStream) {
            break;
        }
    }
    auto const& deserialized_log_events{
            deserializer.get_ir_unit_handler().get_deserialized_log_events()
    };

    std::vector<JsonPair> deserialized_json_pairs;
    for (auto const& log_event : deserialized_log_events) {
        auto const serialized_json_result{log_event.serialize_to_json()};
        REQUIRE_FALSE(serialized_json_result.has_error());
        deserialized_json_pairs.emplace_back(serialized_json_result.value());
    }

    CAPTURE(serialize_json_pairs_to_str(expected_json_pairs));
    CAPTURE(serialize_json_pairs_to_str(deserialized_json_pairs));
    REQUIRE((deserialized_json_pairs == expected_json_pairs));
}
}  // namespace clp::ffi::ir_stream::search::test
