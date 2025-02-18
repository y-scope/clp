#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>
#include <json/single_include/nlohmann/json.hpp>
#include <msgpack.hpp>

#include "../src/clp/BufferReader.hpp"
#include "../src/clp/ErrorCode.hpp"
#include "../src/clp/ffi/encoding_methods.hpp"
#include "../src/clp/ffi/ir_stream/decoding_methods.hpp"
#include "../src/clp/ffi/ir_stream/Deserializer.hpp"
#include "../src/clp/ffi/ir_stream/encoding_methods.hpp"
#include "../src/clp/ffi/ir_stream/IrUnitType.hpp"
#include "../src/clp/ffi/ir_stream/protocol_constants.hpp"
#include "../src/clp/ffi/ir_stream/Serializer.hpp"
#include "../src/clp/ffi/ir_stream/utils.hpp"
#include "../src/clp/ffi/KeyValuePairLogEvent.hpp"
#include "../src/clp/ffi/SchemaTree.hpp"
#include "../src/clp/ir/LogEventDeserializer.hpp"
#include "../src/clp/ir/types.hpp"
#include "../src/clp/time_types.hpp"

using clp::BufferReader;
using clp::enum_to_underlying_type;
using clp::ffi::decode_float_var;
using clp::ffi::decode_integer_var;
using clp::ffi::decode_message;
using clp::ffi::encode_float_string;
using clp::ffi::encode_integer_string;
using clp::ffi::ir_stream::cProtocol::EightByteEncodingMagicNumber;
using clp::ffi::ir_stream::cProtocol::FourByteEncodingMagicNumber;
using clp::ffi::ir_stream::cProtocol::MagicNumberLength;
using clp::ffi::ir_stream::deserialize_preamble;
using clp::ffi::ir_stream::deserialize_tag;
using clp::ffi::ir_stream::deserialize_utc_offset_change;
using clp::ffi::ir_stream::Deserializer;
using clp::ffi::ir_stream::encoded_tag_t;
using clp::ffi::ir_stream::get_encoding_type;
using clp::ffi::ir_stream::IRErrorCode;
using clp::ffi::ir_stream::serialize_utc_offset_change;
using clp::ffi::ir_stream::Serializer;
using clp::ffi::ir_stream::validate_protocol_version;
using clp::ffi::KeyValuePairLogEvent;
using clp::ffi::wildcard_query_matches_any_encoded_var;
using clp::ir::eight_byte_encoded_variable_t;
using clp::ir::epoch_time_ms_t;
using clp::ir::four_byte_encoded_variable_t;
using clp::ir::LogEventDeserializer;
using clp::ir::VariablePlaceholder;
using clp::size_checked_pointer_cast;
using clp::UtcOffset;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;
using std::is_same_v;
using std::string;
using std::string_view;
using std::vector;

namespace {
/**
 * Unstructured log event for testing purposes, consisting of a message, a timestamp, and a UTC
 * offset.
 */
class UnstructuredLogEvent {
public:
    UnstructuredLogEvent(string message, epoch_time_ms_t timestamp, UtcOffset utc_offset)
            : m_message{std::move(message)},
              m_timestamp{timestamp},
              m_utc_offset{utc_offset} {}

    [[nodiscard]] auto get_message() const -> std::string_view { return m_message; }

    [[nodiscard]] auto get_timestamp() const -> epoch_time_ms_t { return m_timestamp; }

    [[nodiscard]] auto get_utc_offset() const -> UtcOffset { return m_utc_offset; }

private:
    string m_message;
    epoch_time_ms_t m_timestamp{0};
    UtcOffset m_utc_offset{0};
};

/**
 * Class that implements `clp::ffi::ir_stream::IrUnitHandlerInterface` for testing purposes.
 */
class IrUnitHandler {
public:
    // Implements `clp::ffi::ir_stream::IrUnitHandlerInterface` interface
    [[nodiscard]] auto handle_log_event(KeyValuePairLogEvent&& log_event) -> IRErrorCode {
        m_deserialized_log_events.emplace_back(std::move(log_event));
        return IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] static auto handle_utc_offset_change(
            [[maybe_unused]] UtcOffset utc_offset_old,
            [[maybe_unused]] UtcOffset utc_offset_new
    ) -> IRErrorCode {
        return IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] static auto handle_schema_tree_node_insertion(
            [[maybe_unused]] bool is_auto_generated,
            [[maybe_unused]] clp::ffi::SchemaTree::NodeLocator schema_tree_node_locator
    ) -> IRErrorCode {
        return IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] auto handle_end_of_stream() -> IRErrorCode {
        m_is_complete = true;
        return IRErrorCode::IRErrorCode_Success;
    }

    // Methods
    [[nodiscard]] auto is_complete() const -> bool { return m_is_complete; }

    [[nodiscard]] auto get_deserialized_log_events() const -> vector<KeyValuePairLogEvent> const& {
        return m_deserialized_log_events;
    }

private:
    vector<KeyValuePairLogEvent> m_deserialized_log_events;
    bool m_is_complete{false};
};

/**
 * Serializes the given log events into an IR buffer.
 * @tparam encoded_variable_t Type of the encoded variables.
 * @param log_events
 * @param preamble_ts
 * @param ir_buf Returns the serialized IR byte sequence.
 * @param encoded_logtypes Returns the encoded logtypes.
 * @return Whether serialization was successful.
 */
template <typename encoded_variable_t>
[[nodiscard]] auto serialize_log_events(
        vector<UnstructuredLogEvent> const& log_events,
        epoch_time_ms_t preamble_ts,
        vector<int8_t>& ir_buf,
        vector<string>& encoded_logtypes
) -> bool;

/**
 * Serializes a log event into an IR buffer.
 * @tparam encoded_variable_t Type of the encoded variables.
 * @param timestamp
 * @param message
 * @param logtype Returns the log event's logtype.
 * @param ir_buf Returns the serialized IR byte sequence.
 * @return Whether serialization was successful.
 */
template <typename encoded_variable_t>
[[nodiscard]] auto serialize_log_event(
        epoch_time_ms_t timestamp,
        string_view message,
        string& logtype,
        vector<int8_t>& ir_buf
) -> bool;

/**
 * @return Log events for testing purposes.
 */
[[nodiscard]] auto create_test_log_events() -> vector<UnstructuredLogEvent>;

/**
 * @return The current UNIX epoch timestamp in milliseconds.
 */
[[nodiscard]] auto get_current_ts() -> epoch_time_ms_t;

/**
 * Flushes and clears serialized data from the serializer's underlying IR buffer into the given byte
 * buffer.
 * @tparam encoded_variable_t
 * @param serializer
 * @param byte_buf
 */
template <typename encoded_variable_t>
auto flush_and_clear_serializer_buffer(
        Serializer<encoded_variable_t>& serializer,
        std::vector<int8_t>& byte_buf
) -> void;

/**
 * Unpacks and serializes the given msgpack bytes using kv serializer.
 * @tparam encoded_variable_t
 * @param auto_gen_msgpack_bytes
 * @param user_gen_msgpack_bytes
 * @param serializer
 * @return Whether serialization succeeded.
 */
template <typename encoded_variable_t>
[[nodiscard]] auto unpack_and_serialize_msgpack_bytes(
        vector<uint8_t> const& auto_gen_msgpack_bytes,
        vector<uint8_t> const& user_gen_msgpack_bytes,
        Serializer<encoded_variable_t>& serializer
) -> bool;

/**
 * @return A msgpack object handle that holds an empty msgpack map.
 */
[[nodiscard]] auto create_msgpack_empty_map_obj_handle() -> msgpack::object_handle;

/**
 * Counts the number of leaves in a JSON tree. A node is considered as a leaf if it's a primitive
 * value, an empty map (`{}`), or an array.
 * @param root
 * @return The number of leaves under the given root.
 */
[[nodiscard]] auto count_num_leaves(nlohmann::json const& root) -> size_t;

/**
 * Unpacks the given bytes into a msgpack object and asserts that serializing it into the KV-pair IR
 * format fails.
 * @tparam encoded_variable_t
 * @param buffer A buffer containing a msgpack byte sequence that cannot be serialized into the
 * KV-pair IR format.
 * @param serializer
 * @return Whether serialization failed and the underlying IR buffer remains empty.
 */
template <typename encoded_variable_t>
[[nodiscard]] auto unpack_and_assert_serialization_failure(
        std::stringstream& buffer,
        Serializer<encoded_variable_t>& serializer
) -> bool;

template <typename encoded_variable_t>
[[nodiscard]] auto serialize_log_events(
        vector<UnstructuredLogEvent> const& log_events,
        epoch_time_ms_t preamble_ts,
        vector<int8_t>& ir_buf,
        vector<string>& encoded_logtypes
) -> bool {
    static_assert(
            (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>)
            || (is_same_v<encoded_variable_t, four_byte_encoded_variable_t>)
    );

    string logtype;
    UtcOffset prev_utc_offset{0};
    epoch_time_ms_t prev_ts{preamble_ts};
    for (auto const& log_event : log_events) {
        auto const ts{log_event.get_timestamp()};
        auto const message{log_event.get_message()};
        auto const utc_offset{log_event.get_utc_offset()};
        auto ts_or_ts_delta{ts};
        if constexpr (is_same_v<encoded_variable_t, four_byte_encoded_variable_t>) {
            ts_or_ts_delta -= prev_ts;
            prev_ts = ts;
        }
        if (utc_offset != prev_utc_offset) {
            clp::ffi::ir_stream::serialize_utc_offset_change(utc_offset, ir_buf);
        }
        if (false
            == serialize_log_event<encoded_variable_t>(ts_or_ts_delta, message, logtype, ir_buf))
        {
            return false;
        }
        encoded_logtypes.emplace_back(logtype);
        prev_utc_offset = utc_offset;
    }
    ir_buf.push_back(clp::ffi::ir_stream::cProtocol::Eof);
    return true;
}

template <typename encoded_variable_t>
auto serialize_log_event(
        epoch_time_ms_t timestamp,
        string_view message,
        string& logtype,
        vector<int8_t>& ir_buf
) -> bool {
    static_assert(
            (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>)
            || (is_same_v<encoded_variable_t, four_byte_encoded_variable_t>)
    );

    if constexpr (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        return clp::ffi::ir_stream::eight_byte_encoding::serialize_log_event(
                timestamp,
                message,
                logtype,
                ir_buf
        );
    } else {
        return clp::ffi::ir_stream::four_byte_encoding::serialize_log_event(
                timestamp,
                message,
                logtype,
                ir_buf
        );
    }
}

auto create_test_log_events() -> vector<UnstructuredLogEvent> {
    vector<UnstructuredLogEvent> log_events;

    log_events.emplace_back(
            "Static <\text>, dictVar1, 123, 456.7, dictVar2, 987, 654.3, end of static text",
            get_current_ts(),
            UtcOffset{0}
    );

    log_events.emplace_back(
            "Static <\text>, dictVar3, 355.2352512, 23953324532112, "
            "python3.4.6, end of static text",
            get_current_ts(),
            UtcOffset{5 * 60 * 60}
    );

    UtcOffset const utc_offset{-5 * 60 * 60};
    log_events.emplace_back("Static text without variables", get_current_ts(), utc_offset);

    log_events.emplace_back(
            "Static text without variable and without utc offset change",
            get_current_ts(),
            utc_offset
    );

    return log_events;
}

auto get_current_ts() -> epoch_time_ms_t {
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

template <typename encoded_variable_t>
auto flush_and_clear_serializer_buffer(
        Serializer<encoded_variable_t>& serializer,
        vector<int8_t>& byte_buf
) -> void {
    auto const view{serializer.get_ir_buf_view()};
    byte_buf.insert(byte_buf.cend(), view.begin(), view.end());
    serializer.clear_ir_buf();
}

template <typename encoded_variable_t>
auto unpack_and_serialize_msgpack_bytes(
        vector<uint8_t> const& auto_gen_msgpack_bytes,
        vector<uint8_t> const& user_gen_msgpack_bytes,
        Serializer<encoded_variable_t>& serializer
) -> bool {
    auto const auto_gen_msgpack_byte_handle{msgpack::unpack(
            clp::size_checked_pointer_cast<char const>(auto_gen_msgpack_bytes.data()),
            auto_gen_msgpack_bytes.size()
    )};
    auto const auto_gen_msgpack_obj{auto_gen_msgpack_byte_handle.get()};
    if (msgpack::type::MAP != auto_gen_msgpack_obj.type) {
        return false;
    }

    auto const user_gen_msgpack_byte_handle{msgpack::unpack(
            clp::size_checked_pointer_cast<char const>(user_gen_msgpack_bytes.data()),
            user_gen_msgpack_bytes.size()
    )};
    auto const user_gen_msgpack_obj{user_gen_msgpack_byte_handle.get()};
    if (msgpack::type::MAP != user_gen_msgpack_obj.type) {
        return false;
    }

    return serializer.serialize_msgpack_map(
            auto_gen_msgpack_obj.via.map,
            user_gen_msgpack_obj.via.map
    );
}

auto create_msgpack_empty_map_obj_handle() -> msgpack::object_handle {
    auto const msgpack_empty_map_buf{nlohmann::json::to_msgpack(nlohmann::json::parse("{}"))};
    return msgpack::unpack(
            size_checked_pointer_cast<char const>(msgpack_empty_map_buf.data()),
            msgpack_empty_map_buf.size()
    );
}

// NOLINTNEXTLINE(misc-no-recursion)
auto count_num_leaves(nlohmann::json const& root) -> size_t {
    if (false == root.is_object()) {
        return 0;
    }

    size_t num_leaves{0};
    for (auto const& [key, val] : root.items()) {
        if (val.is_primitive() || val.is_array()) {
            ++num_leaves;
        } else if (val.is_object()) {
            if (val.empty()) {
                ++num_leaves;
            } else {
                num_leaves += count_num_leaves(val);
            }
        } else {
            FAIL("Unknown JSON object types.");
        }
    }

    return num_leaves;
}

template <typename encoded_variable_t>
auto unpack_and_assert_serialization_failure(
        std::stringstream& buffer,
        Serializer<encoded_variable_t>& serializer
) -> bool {
    REQUIRE(serializer.get_ir_buf_view().empty());
    string msgpack_bytes{buffer.str()};
    buffer.str({});
    buffer.clear();
    auto const msgpack_obj_handle{msgpack::unpack(msgpack_bytes.data(), msgpack_bytes.size())};
    auto const msgpack_obj{msgpack_obj_handle.get()};
    REQUIRE((msgpack::type::MAP == msgpack_obj.type));

    auto const msgpack_empty_map_obj_handle{create_msgpack_empty_map_obj_handle()};
    auto const msgpack_empty_map_obj{msgpack_empty_map_obj_handle.get()};

    if (serializer.serialize_msgpack_map(msgpack_obj.via.map, msgpack_empty_map_obj.via.map)) {
        // Serialization should fail
        return false;
    }
    if (serializer.serialize_msgpack_map(msgpack_empty_map_obj.via.map, msgpack_obj.via.map)) {
        // Serialization should fail
        return false;
    }
    if (false == serializer.get_ir_buf_view().empty()) {
        // Serialization buffer should be empty
        return false;
    }
    return true;
}
}  // namespace

/**
 * @tparam encoded_variable_t Type of the encoded variable
 * @param is_four_bytes_encoding
 * @return True if input encoding type matches the type of encoded_variable_t false otherwise
 */
template <typename encoded_variable_t>
bool match_encoding_type(bool is_four_bytes_encoding);

template <typename encoded_variable_t>
epoch_time_ms_t get_next_timestamp_for_test();

/**
 * Helper function that serializes a preamble of encoding type = encoded_variable_t and writes into
 * ir_buf
 * @tparam encoded_variable_t Type of the encoded variable
 * @param timestamp_pattern
 * @param timestamp_pattern_syntax
 * @param time_zone_id
 * @param reference_timestamp Only used when encoded_variable_t == four_byte_encoded_variable_t
 * @param ir_buf
 * @return True if the preamble is serialized without error, otherwise false
 */
template <typename encoded_variable_t>
bool serialize_preamble(
        string_view timestamp_pattern,
        string_view timestamp_pattern_syntax,
        string_view time_zone_id,
        epoch_time_ms_t reference_timestamp,
        vector<int8_t>& ir_buf
);

/**
 * Helper function that deserializes a log event of encoding type = encoded_variable_t from the
 * ir_buf
 * @tparam encoded_variable_t Type of the encoded variable
 * @param reader
 * @param tag
 * @param message
 * @param decoded_ts Returns the decoded timestamp
 * @return IRErrorCode_Success on success
 * @return Same as the clp::ffi::ir_stream::eight_byte_encoding::deserialize_log_event when
 * encoded_variable_t == eight_byte_encoded_variable_t
 * @return Same as the clp::ffi::ir_stream::four_byte_encoding::deserialize_log_event when
 * encoded_variable_t == four_byte_encoded_variable_t
 */
template <typename encoded_variable_t>
IRErrorCode deserialize_log_event(
        BufferReader& reader,
        encoded_tag_t tag,
        string& message,
        epoch_time_ms_t& decoded_ts
);

/**
 * Struct to hold the timestamp info from the IR stream's metadata
 */
struct TimestampInfo {
    std::string timestamp_pattern;
    std::string timestamp_pattern_syntax;
    std::string time_zone_id;
};

/**
 * Extracts timestamp info from the JSON metadata and stores it into ts_info
 * @param metadata_json The JSON metadata
 * @param ts_info Returns the timestamp info
 */
static void set_timestamp_info(nlohmann::json const& metadata_json, TimestampInfo& ts_info);

template <typename encoded_variable_t>
bool match_encoding_type(bool is_four_bytes_encoding) {
    static_assert(
            (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>)
            || (is_same_v<encoded_variable_t, four_byte_encoded_variable_t>)
    );

    if constexpr (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        return false == is_four_bytes_encoding;
    } else {
        return is_four_bytes_encoding;
    }
}

template <typename encoded_variable_t>
epoch_time_ms_t get_next_timestamp_for_test() {
    static_assert(
            (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>)
            || (is_same_v<encoded_variable_t, four_byte_encoded_variable_t>)
    );

    // We return an absolute timestamp for the eight-byte encoding and a mocked timestamp delta for
    // the four-byte encoding
    if constexpr (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        return get_current_ts();
    } else {
        epoch_time_ms_t ts1 = get_current_ts();
        epoch_time_ms_t ts2 = get_current_ts();
        return ts2 - ts1;
    }
}

// A helper function to generalize the testing caller interface.
// The reference_timestamp is only used by four bytes encoding
template <typename encoded_variable_t>
bool serialize_preamble(
        string_view timestamp_pattern,
        string_view timestamp_pattern_syntax,
        string_view time_zone_id,
        epoch_time_ms_t reference_timestamp,
        vector<int8_t>& ir_buf
) {
    static_assert(
            (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>)
            || (is_same_v<encoded_variable_t, four_byte_encoded_variable_t>)
    );

    if constexpr (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        return clp::ffi::ir_stream::eight_byte_encoding::serialize_preamble(
                timestamp_pattern,
                timestamp_pattern_syntax,
                time_zone_id,
                ir_buf
        );
    } else {
        return clp::ffi::ir_stream::four_byte_encoding::serialize_preamble(
                timestamp_pattern,
                timestamp_pattern_syntax,
                time_zone_id,
                reference_timestamp,
                ir_buf
        );
    }
}

template <typename encoded_variable_t>
IRErrorCode deserialize_log_event(
        BufferReader& reader,
        encoded_tag_t tag,
        string& message,
        epoch_time_ms_t& decoded_ts
) {
    static_assert(
            (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>)
            || (is_same_v<encoded_variable_t, four_byte_encoded_variable_t>)
    );

    if constexpr (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        return clp::ffi::ir_stream::eight_byte_encoding::deserialize_log_event(
                reader,
                tag,
                message,
                decoded_ts
        );
    } else {
        return clp::ffi::ir_stream::four_byte_encoding::deserialize_log_event(
                reader,
                tag,
                message,
                decoded_ts
        );
    }
}

static void set_timestamp_info(nlohmann::json const& metadata_json, TimestampInfo& ts_info) {
    ts_info.time_zone_id
            = metadata_json.at(clp::ffi::ir_stream::cProtocol::Metadata::TimeZoneIdKey);
    ts_info.timestamp_pattern
            = metadata_json.at(clp::ffi::ir_stream::cProtocol::Metadata::TimestampPatternKey);
    ts_info.timestamp_pattern_syntax
            = metadata_json.at(clp::ffi::ir_stream::cProtocol::Metadata::TimestampPatternSyntaxKey);
}

TEST_CASE("get_encoding_type", "[ffi][get_encoding_type]") {
    bool is_four_bytes_encoding;

    // Test eight-byte encoding
    vector<int8_t> eight_byte_encoding_vec{
            EightByteEncodingMagicNumber,
            EightByteEncodingMagicNumber + MagicNumberLength
    };

    BufferReader eight_byte_ir_buffer{
            size_checked_pointer_cast<char const>(eight_byte_encoding_vec.data()),
            eight_byte_encoding_vec.size()
    };
    REQUIRE(get_encoding_type(eight_byte_ir_buffer, is_four_bytes_encoding)
            == IRErrorCode::IRErrorCode_Success);
    REQUIRE(match_encoding_type<eight_byte_encoded_variable_t>(is_four_bytes_encoding));

    // Test four-byte encoding
    vector<int8_t> four_byte_encoding_vec{
            FourByteEncodingMagicNumber,
            FourByteEncodingMagicNumber + MagicNumberLength
    };

    BufferReader four_byte_ir_buffer{
            size_checked_pointer_cast<char const>(four_byte_encoding_vec.data()),
            four_byte_encoding_vec.size()
    };
    REQUIRE(get_encoding_type(four_byte_ir_buffer, is_four_bytes_encoding)
            == IRErrorCode::IRErrorCode_Success);
    REQUIRE(match_encoding_type<four_byte_encoded_variable_t>(is_four_bytes_encoding));

    // Test error on empty and incomplete ir_buffer
    BufferReader empty_ir_buffer(
            size_checked_pointer_cast<char const>(four_byte_encoding_vec.data()),
            0
    );
    REQUIRE(get_encoding_type(empty_ir_buffer, is_four_bytes_encoding)
            == IRErrorCode::IRErrorCode_Incomplete_IR);

    BufferReader incomplete_buffer{
            size_checked_pointer_cast<char const>(four_byte_encoding_vec.data()),
            four_byte_encoding_vec.size() - 1
    };
    REQUIRE(get_encoding_type(incomplete_buffer, is_four_bytes_encoding)
            == IRErrorCode::IRErrorCode_Incomplete_IR);

    // Test error on invalid encoding
    vector<int8_t> const invalid_ir_vec{0x02, 0x43, 0x24, 0x34};
    BufferReader invalid_ir_buffer{
            size_checked_pointer_cast<char const>(invalid_ir_vec.data()),
            invalid_ir_vec.size()
    };
    REQUIRE(get_encoding_type(invalid_ir_buffer, is_four_bytes_encoding)
            == IRErrorCode::IRErrorCode_Corrupted_IR);
}

TEMPLATE_TEST_CASE(
        "deserialize_preamble",
        "[ffi][deserialize_preamble]",
        four_byte_encoded_variable_t,
        eight_byte_encoded_variable_t
) {
    vector<int8_t> ir_buf;
    constexpr char timestamp_pattern[] = "%Y-%m-%d %H:%M:%S,%3";
    constexpr char timestamp_pattern_syntax[] = "yyyy-MM-dd HH:mm:ss";
    constexpr char time_zone_id[] = "Asia/Tokyo";
    epoch_time_ms_t const reference_ts = get_current_ts();
    REQUIRE(serialize_preamble<TestType>(
            timestamp_pattern,
            timestamp_pattern_syntax,
            time_zone_id,
            reference_ts,
            ir_buf
    ));
    size_t const encoded_preamble_end_pos = ir_buf.size();

    // Check if encoding type is properly read
    BufferReader ir_buffer{size_checked_pointer_cast<char const>(ir_buf.data()), ir_buf.size()};
    bool is_four_bytes_encoding;
    REQUIRE(get_encoding_type(ir_buffer, is_four_bytes_encoding) == IRErrorCode::IRErrorCode_Success
    );
    REQUIRE(match_encoding_type<TestType>(is_four_bytes_encoding));
    REQUIRE(MagicNumberLength == ir_buffer.get_pos());

    // Test if preamble can be decoded correctly
    TimestampInfo ts_info;
    encoded_tag_t metadata_type{0};
    size_t metadata_pos{0};
    uint16_t metadata_size{0};
    REQUIRE(deserialize_preamble(ir_buffer, metadata_type, metadata_pos, metadata_size)
            == IRErrorCode::IRErrorCode_Success);
    REQUIRE(encoded_preamble_end_pos == ir_buffer.get_pos());

    char* metadata_ptr{size_checked_pointer_cast<char>(ir_buf.data()) + metadata_pos};
    string_view json_metadata{metadata_ptr, metadata_size};

    auto metadata_json = nlohmann::json::parse(json_metadata);
    std::string const version
            = metadata_json.at(clp::ffi::ir_stream::cProtocol::Metadata::VersionKey);
    REQUIRE(clp::ffi::ir_stream::IRProtocolErrorCode::BackwardCompatible
            == validate_protocol_version(version));
    REQUIRE(clp::ffi::ir_stream::cProtocol::Metadata::EncodingJson == metadata_type);
    set_timestamp_info(metadata_json, ts_info);
    REQUIRE(timestamp_pattern_syntax == ts_info.timestamp_pattern_syntax);
    REQUIRE(time_zone_id == ts_info.time_zone_id);
    REQUIRE(timestamp_pattern == ts_info.timestamp_pattern);
    REQUIRE(encoded_preamble_end_pos == ir_buffer.get_pos());

    if constexpr (is_same_v<TestType, four_byte_encoded_variable_t>) {
        REQUIRE(reference_ts
                == std::stoll(
                        metadata_json
                                .at(clp::ffi::ir_stream::cProtocol::Metadata::ReferenceTimestampKey)
                                .get<string>()
                ));
    }

    // Test if preamble can be decoded by the string copy method
    std::vector<int8_t> json_metadata_vec;
    ir_buffer.seek_from_begin(MagicNumberLength);
    REQUIRE(deserialize_preamble(ir_buffer, metadata_type, json_metadata_vec)
            == IRErrorCode::IRErrorCode_Success);
    string_view json_metadata_copied{
            size_checked_pointer_cast<char const>(json_metadata_vec.data()),
            json_metadata_vec.size()
    };
    // Crosscheck with the json_metadata decoded previously
    REQUIRE(json_metadata_copied == json_metadata);

    // Test if incomplete IR can be detected
    ir_buf.resize(encoded_preamble_end_pos - 1);
    BufferReader incomplete_preamble_buffer{
            size_checked_pointer_cast<char const>(ir_buf.data()),
            ir_buf.size()
    };
    incomplete_preamble_buffer.seek_from_begin(MagicNumberLength);
    REQUIRE(deserialize_preamble(
                    incomplete_preamble_buffer,
                    metadata_type,
                    metadata_pos,
                    metadata_size
            )
            == IRErrorCode::IRErrorCode_Incomplete_IR);

    // Test if corrupted IR can be detected
    ir_buf[MagicNumberLength] = 0x23;
    BufferReader corrupted_preamble_buffer{
            size_checked_pointer_cast<char const>(ir_buf.data()),
            ir_buf.size()
    };
    REQUIRE(deserialize_preamble(
                    corrupted_preamble_buffer,
                    metadata_type,
                    metadata_pos,
                    metadata_size
            )
            == IRErrorCode::IRErrorCode_Corrupted_IR);
}

TEMPLATE_TEST_CASE(
        "decode_next_message_general",
        "[ffi][deserialize_log_event]",
        four_byte_encoded_variable_t,
        eight_byte_encoded_variable_t
) {
    vector<int8_t> ir_buf;
    string logtype;

    string placeholder_as_string{enum_to_underlying_type(VariablePlaceholder::Dictionary)};
    string message = "Static <\text>, dictVar1, 123, 456.7 dictVar2, 987, 654.3,"
                     + placeholder_as_string + " end of static text";
    epoch_time_ms_t reference_timestamp = get_next_timestamp_for_test<TestType>();
    REQUIRE(true == serialize_log_event<TestType>(reference_timestamp, message, logtype, ir_buf));
    size_t const encoded_message_end_pos = ir_buf.size();
    size_t const encoded_message_start_pos = 0;

    BufferReader ir_buffer{size_checked_pointer_cast<char const>(ir_buf.data()), ir_buf.size()};
    string decoded_message;
    epoch_time_ms_t timestamp;
    encoded_tag_t tag;

    // Test if message can be decoded properly
    REQUIRE(IRErrorCode::IRErrorCode_Success == deserialize_tag(ir_buffer, tag));
    REQUIRE(IRErrorCode::IRErrorCode_Success
            == deserialize_log_event<TestType>(ir_buffer, tag, decoded_message, timestamp));
    REQUIRE(message == decoded_message);
    REQUIRE(timestamp == reference_timestamp);
    REQUIRE(ir_buffer.get_pos() == encoded_message_end_pos);

    // Test corrupted IR
    ir_buffer.seek_from_begin(encoded_message_start_pos + 1);
    REQUIRE(IRErrorCode::IRErrorCode_Success == deserialize_tag(ir_buffer, tag));
    REQUIRE(IRErrorCode::IRErrorCode_Corrupted_IR
            == deserialize_log_event<TestType>(ir_buffer, tag, message, timestamp));

    // Test incomplete IR
    ir_buf.resize(encoded_message_end_pos - 4);
    BufferReader incomplete_preamble_buffer{
            size_checked_pointer_cast<char const>(ir_buf.data()),
            ir_buf.size()
    };
    REQUIRE(IRErrorCode::IRErrorCode_Success == deserialize_tag(incomplete_preamble_buffer, tag));
    REQUIRE(IRErrorCode::IRErrorCode_Incomplete_IR
            == deserialize_log_event<TestType>(incomplete_preamble_buffer, tag, message, timestamp)
    );
}

// NOTE: This test only tests eight_byte_encoded_variable_t because we trigger
// IRErrorCode_Decode_Error by manually modifying the logtype within the IR, and this is easier for
// the eight_byte_encoded_variable_t case.
TEST_CASE("message_decode_error", "[ffi][deserialize_log_event]") {
    vector<int8_t> ir_buf;
    string logtype;

    string placeholder_as_string{enum_to_underlying_type(VariablePlaceholder::Dictionary)};
    string message = "Static <\text>, dictVar1, 123, 456.7 dictVar2, 987, 654.3,"
                     + placeholder_as_string + " end of static text";
    epoch_time_ms_t reference_ts = get_next_timestamp_for_test<eight_byte_encoded_variable_t>();
    REQUIRE(true
            == serialize_log_event<eight_byte_encoded_variable_t>(
                    reference_ts,
                    message,
                    logtype,
                    ir_buf
            ));

    // Find the end of the encoded logtype which is before the encoded timestamp
    // The timestamp is encoded as tagbyte + eight_byte_encoded_variable_t
    size_t timestamp_encoding_size = sizeof(clp::ffi::ir_stream::cProtocol::Payload::TimestampVal)
                                     + sizeof(eight_byte_encoded_variable_t);
    size_t const logtype_end_pos = ir_buf.size() - timestamp_encoding_size;

    string decoded_message;
    epoch_time_ms_t timestamp;

    // Test if a trailing escape triggers a decoder error
    auto ir_with_extra_escape{ir_buf};
    encoded_tag_t tag;
    ir_with_extra_escape.at(logtype_end_pos - 1)
            = enum_to_underlying_type(VariablePlaceholder::Escape);
    BufferReader ir_with_extra_escape_buffer{
            size_checked_pointer_cast<char const>(ir_with_extra_escape.data()),
            ir_with_extra_escape.size()
    };
    REQUIRE(IRErrorCode::IRErrorCode_Success == deserialize_tag(ir_with_extra_escape_buffer, tag));
    REQUIRE(IRErrorCode::IRErrorCode_Decode_Error
            == deserialize_log_event<eight_byte_encoded_variable_t>(
                    ir_with_extra_escape_buffer,
                    tag,
                    decoded_message,
                    timestamp
            ));

    // Test if an extra placeholder triggers a decoder error
    auto ir_with_extra_placeholder{ir_buf};
    ir_with_extra_placeholder.at(logtype_end_pos - 1)
            = enum_to_underlying_type(VariablePlaceholder::Dictionary);
    BufferReader ir_with_extra_placeholder_buffer{
            size_checked_pointer_cast<char const>(ir_with_extra_placeholder.data()),
            ir_with_extra_placeholder.size()
    };
    REQUIRE(IRErrorCode::IRErrorCode_Success
            == deserialize_tag(ir_with_extra_placeholder_buffer, tag));
    REQUIRE(IRErrorCode::IRErrorCode_Decode_Error
            == deserialize_log_event<eight_byte_encoded_variable_t>(
                    ir_with_extra_placeholder_buffer,
                    tag,
                    decoded_message,
                    timestamp
            ));
}

TEST_CASE("decode_next_message_four_byte_timestamp_delta", "[ffi][deserialize_log_event]") {
    string const message = "Static <\text>, dictVar1, 123, 456345232.7234223, "
                           "dictVar2, 987, 654.3, end of static text";
    auto ts_delta = GENERATE(
            static_cast<epoch_time_ms_t>(0),
            static_cast<epoch_time_ms_t>(INT8_MIN),
            static_cast<epoch_time_ms_t>(INT8_MIN + 1),
            static_cast<epoch_time_ms_t>(INT8_MAX - 1),
            static_cast<epoch_time_ms_t>(INT8_MAX),
            static_cast<epoch_time_ms_t>(INT16_MIN),
            static_cast<epoch_time_ms_t>(INT16_MIN + 1),
            static_cast<epoch_time_ms_t>(INT16_MAX - 1),
            static_cast<epoch_time_ms_t>(INT16_MAX),
            static_cast<epoch_time_ms_t>(INT32_MIN),
            static_cast<epoch_time_ms_t>(INT32_MIN + 1),
            static_cast<epoch_time_ms_t>(INT32_MAX - 1),
            static_cast<epoch_time_ms_t>(INT32_MAX),
            static_cast<epoch_time_ms_t>(INT64_MIN),
            static_cast<epoch_time_ms_t>(INT64_MAX)
    );
    vector<int8_t> ir_buf;
    string logtype;
    REQUIRE(serialize_log_event<four_byte_encoded_variable_t>(ts_delta, message, logtype, ir_buf));

    BufferReader ir_buffer{size_checked_pointer_cast<char const>(ir_buf.data()), ir_buf.size()};
    string decoded_message;
    encoded_tag_t tag;
    epoch_time_ms_t decoded_delta_ts{};
    REQUIRE(IRErrorCode::IRErrorCode_Success == deserialize_tag(ir_buffer, tag));
    REQUIRE(IRErrorCode::IRErrorCode_Success
            == deserialize_log_event<four_byte_encoded_variable_t>(
                    ir_buffer,
                    tag,
                    decoded_message,
                    decoded_delta_ts
            ));
    REQUIRE(message == decoded_message);
    REQUIRE(decoded_delta_ts == ts_delta);
}

TEST_CASE("validate_protocol_version", "[ffi][validate_version_protocol]") {
    REQUIRE(
            (clp::ffi::ir_stream::IRProtocolErrorCode::Supported
             == validate_protocol_version(clp::ffi::ir_stream::cProtocol::Metadata::VersionValue))
    );
    REQUIRE(
            (clp::ffi::ir_stream::IRProtocolErrorCode::BackwardCompatible
             == validate_protocol_version(
                     clp::ffi::ir_stream::cProtocol::Metadata::LatestBackwardCompatibleVersion
             ))
    );

    SECTION("Test invalid versions") {
        auto const invalid_versions{GENERATE(
                std::string_view{"v0.0.1"},
                std::string_view{"0.1"},
                std::string_view{"0.1.a"},
                std::string_view{"0.a.1"}
        )};
        REQUIRE(
                (clp::ffi::ir_stream::IRProtocolErrorCode::Invalid
                 == validate_protocol_version(invalid_versions))
        );
    }

    SECTION("Test backward compatible versions") {
        auto const backward_compatible_versions{GENERATE(
                std::string_view{"v0.0.0"},
                std::string_view{"0.0.1"},
                std::string_view{"0.0.2"}
        )};
        REQUIRE(
                (clp::ffi::ir_stream::IRProtocolErrorCode::BackwardCompatible
                 == validate_protocol_version(backward_compatible_versions))
        );
    }

    SECTION("Test versions that're too old") {
        auto const old_versions{GENERATE(
                std::string_view{"0.0.3"},
                std::string_view{"0.0.3-beta.1"},
                std::string_view{"0.1.0-beta"}
        )};
        REQUIRE(
                (clp::ffi::ir_stream::IRProtocolErrorCode::Unsupported
                 == validate_protocol_version(old_versions))
        );
    }

    SECTION("Test versions that're too new") {
        auto const new_versions{
                GENERATE(std::string_view{"10000.0.0"}, std::string_view{"0.10000.0"})
        };
        REQUIRE(
                (clp::ffi::ir_stream::IRProtocolErrorCode::Unsupported
                 == validate_protocol_version(new_versions))
        );
    }
}

TEMPLATE_TEST_CASE(
        "decode_ir_complete",
        "[ffi][deserialize_log_event]",
        four_byte_encoded_variable_t,
        eight_byte_encoded_variable_t
) {
    vector<int8_t> ir_buf;

    epoch_time_ms_t const preamble_ts = get_current_ts();
    constexpr char timestamp_pattern[] = "%Y-%m-%d %H:%M:%S,%3";
    constexpr char timestamp_pattern_syntax[] = "yyyy-MM-dd HH:mm:ss";
    constexpr char time_zone_id[] = "Asia/Tokyo";
    REQUIRE(serialize_preamble<TestType>(
            timestamp_pattern,
            timestamp_pattern_syntax,
            time_zone_id,
            preamble_ts,
            ir_buf
    ));
    auto const encoded_preamble_end_pos = ir_buf.size();

    auto const test_log_events{create_test_log_events()};
    vector<string> encoded_logtypes;
    REQUIRE(serialize_log_events<TestType>(test_log_events, preamble_ts, ir_buf, encoded_logtypes));

    BufferReader complete_ir_buffer{
            size_checked_pointer_cast<char const>(ir_buf.data()),
            ir_buf.size()
    };

    bool is_four_bytes_encoding;
    REQUIRE(get_encoding_type(complete_ir_buffer, is_four_bytes_encoding)
            == IRErrorCode::IRErrorCode_Success);
    REQUIRE(match_encoding_type<TestType>(is_four_bytes_encoding));

    // Test if preamble can be properly decoded
    TimestampInfo ts_info;
    encoded_tag_t metadata_type;
    size_t metadata_pos;
    uint16_t metadata_size;
    REQUIRE(deserialize_preamble(complete_ir_buffer, metadata_type, metadata_pos, metadata_size)
            == IRErrorCode::IRErrorCode_Success);
    REQUIRE(encoded_preamble_end_pos == complete_ir_buffer.get_pos());

    auto* json_metadata_ptr{size_checked_pointer_cast<char>(ir_buf.data() + metadata_pos)};
    string_view json_metadata{json_metadata_ptr, metadata_size};
    auto metadata_json = nlohmann::json::parse(json_metadata);
    string const version = metadata_json.at(clp::ffi::ir_stream::cProtocol::Metadata::VersionKey);
    REQUIRE(clp::ffi::ir_stream::IRProtocolErrorCode::BackwardCompatible
            == validate_protocol_version(version));
    REQUIRE(clp::ffi::ir_stream::cProtocol::Metadata::EncodingJson == metadata_type);
    set_timestamp_info(metadata_json, ts_info);
    REQUIRE(timestamp_pattern_syntax == ts_info.timestamp_pattern_syntax);
    REQUIRE(time_zone_id == ts_info.time_zone_id);
    REQUIRE(timestamp_pattern == ts_info.timestamp_pattern);

    string decoded_message;
    epoch_time_ms_t ts_or_ts_delta{};
    UtcOffset utc_offset{0};
    encoded_tag_t tag;
    epoch_time_ms_t prev_ts{preamble_ts};
    for (auto const& log_event : test_log_events) {
        REQUIRE(IRErrorCode::IRErrorCode_Success == deserialize_tag(complete_ir_buffer, tag));
        if (clp::ffi::ir_stream::cProtocol::Payload::UtcOffsetChange == tag) {
            REQUIRE(IRErrorCode::IRErrorCode_Success
                    == deserialize_utc_offset_change(complete_ir_buffer, utc_offset));
            REQUIRE(IRErrorCode::IRErrorCode_Success == deserialize_tag(complete_ir_buffer, tag));
        }

        REQUIRE(IRErrorCode::IRErrorCode_Success
                == deserialize_log_event<TestType>(
                        complete_ir_buffer,
                        tag,
                        decoded_message,
                        ts_or_ts_delta
                ));
        auto timestamp{ts_or_ts_delta};
        if constexpr (is_same_v<TestType, four_byte_encoded_variable_t>) {
            timestamp += prev_ts;
            prev_ts = timestamp;
        }
        REQUIRE(log_event.get_message() == decoded_message);
        REQUIRE(log_event.get_timestamp() == timestamp);
        REQUIRE(log_event.get_utc_offset() == utc_offset);
    }
    REQUIRE(IRErrorCode::IRErrorCode_Success == deserialize_tag(complete_ir_buffer, tag));
    REQUIRE(clp::ffi::ir_stream::cProtocol::Eof == tag);
    REQUIRE(complete_ir_buffer.get_pos() == ir_buf.size());
}

TEMPLATE_TEST_CASE(
        "clp::ir::LogEventDeserializer",
        "[clp][ir][LogEventDeserializer]",
        four_byte_encoded_variable_t,
        eight_byte_encoded_variable_t
) {
    vector<int8_t> ir_buf;

    epoch_time_ms_t preamble_ts = get_current_ts();
    constexpr char timestamp_pattern[] = "%Y-%m-%d %H:%M:%S,%3";
    constexpr char timestamp_pattern_syntax[] = "yyyy-MM-dd HH:mm:ss";
    constexpr char time_zone_id[] = "Asia/Tokyo";
    REQUIRE(serialize_preamble<TestType>(
            timestamp_pattern,
            timestamp_pattern_syntax,
            time_zone_id,
            preamble_ts,
            ir_buf
    ));

    auto const test_log_events{create_test_log_events()};
    vector<string> encoded_logtypes;
    REQUIRE(serialize_log_events<TestType>(test_log_events, preamble_ts, ir_buf, encoded_logtypes));

    BufferReader complete_ir_buffer{
            size_checked_pointer_cast<char const>(ir_buf.data()),
            ir_buf.size()
    };

    bool is_four_bytes_encoding;
    REQUIRE(get_encoding_type(complete_ir_buffer, is_four_bytes_encoding)
            == IRErrorCode::IRErrorCode_Success);
    REQUIRE(match_encoding_type<TestType>(is_four_bytes_encoding));

    auto create_result = LogEventDeserializer<TestType>::create(complete_ir_buffer);
    REQUIRE(false == create_result.has_error());
    auto& log_event_deserializer = create_result.value();
    size_t log_event_idx{0};
    for (auto const& ref_log_event : test_log_events) {
        auto result{log_event_deserializer.deserialize_log_event()};
        REQUIRE(false == result.has_error());
        REQUIRE(log_event_deserializer.get_current_utc_offset() == ref_log_event.get_utc_offset());
        auto const& log_event{result.value()};
        REQUIRE(log_event.get_timestamp() == ref_log_event.get_timestamp());
        REQUIRE(log_event.get_utc_offset() == ref_log_event.get_utc_offset());
        // We only compare the logtype since decoding messages from logtype + variables is not yet
        // supported by our public interfaces
        REQUIRE(log_event.get_message().get_logtype() == encoded_logtypes.at(log_event_idx));
        ++log_event_idx;
    }
    auto result = log_event_deserializer.deserialize_log_event();
    REQUIRE(result.has_error());
    REQUIRE(std::errc::no_message_available == result.error());
}

TEMPLATE_TEST_CASE(
        "ffi_ir_stream_Serializer_creation",
        "[clp][ffi][ir_stream][Serializer]",
        four_byte_encoded_variable_t,
        eight_byte_encoded_variable_t
) {
    // This is a unit test for the kv-pair IR serializer. Currently, we haven't yet implemented a
    // deserializer, so we can only test whether the preamble packet is serialized correctly.
    vector<int8_t> ir_buf;

    auto result{Serializer<TestType>::create()};
    REQUIRE((false == result.has_error()));

    auto& serializer{result.value()};
    flush_and_clear_serializer_buffer(serializer, ir_buf);
    REQUIRE(serializer.get_ir_buf_view().empty());

    constexpr UtcOffset cBeijingUtcOffset{8 * 60 * 60 * 1000};
    serializer.change_utc_offset(cBeijingUtcOffset);
    flush_and_clear_serializer_buffer(serializer, ir_buf);
    REQUIRE(serializer.get_ir_buf_view().empty());

    ir_buf.push_back(clp::ffi::ir_stream::cProtocol::Eof);

    BufferReader buffer_reader{size_checked_pointer_cast<char const>(ir_buf.data()), ir_buf.size()};

    bool is_four_byte_encoding{};
    REQUIRE(
            (IRErrorCode::IRErrorCode_Success
             == get_encoding_type(buffer_reader, is_four_byte_encoding))
    );
    if constexpr (std::is_same_v<TestType, four_byte_encoded_variable_t>) {
        REQUIRE(is_four_byte_encoding);
    } else {
        REQUIRE((false == is_four_byte_encoding));
    }

    encoded_tag_t metadata_type{};
    vector<int8_t> metadata_bytes;
    REQUIRE(
            (IRErrorCode::IRErrorCode_Success
             == deserialize_preamble(buffer_reader, metadata_type, metadata_bytes))
    );
    REQUIRE((clp::ffi::ir_stream::cProtocol::Metadata::EncodingJson == metadata_type));
    string_view const metadata_view{
            size_checked_pointer_cast<char const>(metadata_bytes.data()),
            metadata_bytes.size()
    };
    nlohmann::json const metadata = nlohmann::json::parse(metadata_view);

    nlohmann::json expected_metadata;
    expected_metadata.emplace(
            clp::ffi::ir_stream::cProtocol::Metadata::VersionKey,
            clp::ffi::ir_stream::cProtocol::Metadata::VersionValue
    );
    expected_metadata.emplace(
            clp::ffi::ir_stream::cProtocol::Metadata::VariablesSchemaIdKey,
            clp::ffi::cVariablesSchemaVersion
    );
    expected_metadata.emplace(
            clp::ffi::ir_stream::cProtocol::Metadata::VariableEncodingMethodsIdKey,
            clp::ffi::cVariableEncodingMethodsVersion
    );
    REQUIRE((expected_metadata == metadata));

    encoded_tag_t encoded_tag{};
    REQUIRE((IRErrorCode::IRErrorCode_Success == deserialize_tag(buffer_reader, encoded_tag)));
    REQUIRE((clp::ffi::ir_stream::cProtocol::Payload::UtcOffsetChange == encoded_tag));
    UtcOffset utc_offset_change{0};
    REQUIRE(
            (IRErrorCode::IRErrorCode_Success
             == deserialize_utc_offset_change(buffer_reader, utc_offset_change))
    );
    REQUIRE((cBeijingUtcOffset == utc_offset_change));

    REQUIRE((IRErrorCode::IRErrorCode_Success == deserialize_tag(buffer_reader, encoded_tag)));
    REQUIRE((clp::ffi::ir_stream::cProtocol::Eof == encoded_tag));

    char eof{};
    size_t num_bytes_read{};
    REQUIRE(
            (clp::ErrorCode_EndOfFile == buffer_reader.try_read(&eof, 1, num_bytes_read)
             && 0 == num_bytes_read)
    );
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEMPLATE_TEST_CASE(
        "ffi_ir_stream_kv_pair_log_events_serde",
        "[clp][ffi][ir_stream]",
        four_byte_encoded_variable_t,
        eight_byte_encoded_variable_t
) {
    vector<int8_t> ir_buf;
    vector<std::pair<nlohmann::json, nlohmann::json>> expected_auto_gen_and_user_gen_object_pairs;

    nlohmann::json const user_defined_metadata
            = {{"map", {{"int", 0}, {"str", "STRING"}}}, {"array", {0, 0.0, true, "String"}}};
    auto result{Serializer<TestType>::create(user_defined_metadata)};
    REQUIRE((false == result.has_error()));

    auto& serializer{result.value()};
    flush_and_clear_serializer_buffer(serializer, ir_buf);

    auto const empty_obj = nlohmann::json::parse("{}");
    expected_auto_gen_and_user_gen_object_pairs.emplace_back(empty_obj, empty_obj);

    // Test encoding basic object
    constexpr string_view cShortString{"short_string"};
    constexpr string_view cClpString{"uid=0, CPU usage: 99.99%, \"user_name\"=YScope"};
    auto const empty_array = nlohmann::json::parse("[]");
    nlohmann::json const basic_obj
            = {{"int8_max", INT8_MAX},
               {"int8_min", INT8_MIN},
               {"int16_max", INT16_MAX},
               {"int16_min", INT16_MIN},
               {"int32_max", INT32_MAX},
               {"int32_min", INT32_MIN},
               {"int64_max", INT64_MAX},
               {"int64_min", INT64_MIN},
               {"float_zero", 0.0},
               {"float_pos", 1.01},
               {"float_neg", -1.01},
               {"true", true},
               {"false", false},
               {"string", cShortString},
               {"clp_string", cClpString},
               {"null", nullptr},
               {"empty_object", empty_obj},
               {"empty_array", empty_array}};
    expected_auto_gen_and_user_gen_object_pairs.emplace_back(basic_obj, basic_obj);

    auto basic_array = empty_array;
    basic_array.emplace_back(1);
    basic_array.emplace_back(1.0);
    basic_array.emplace_back(true);
    basic_array.emplace_back(cShortString);
    basic_array.emplace_back(cClpString);
    basic_array.emplace_back(nullptr);
    basic_array.emplace_back(empty_array);
    for (auto const& element : basic_array) {
        // Non-map objects should not be serializable
        REQUIRE(
                (false
                 == unpack_and_serialize_msgpack_bytes(
                         nlohmann::json::to_msgpack(empty_obj),
                         nlohmann::json::to_msgpack(element),
                         serializer
                 ))
        );
    }
    basic_array.emplace_back(empty_obj);

    // Recursively construct an object containing inner maps and inner arrays.
    auto recursive_obj = basic_obj;
    auto recursive_array = basic_array;
    constexpr size_t cRecursiveDepth{6};
    for (size_t i{0}; i < cRecursiveDepth; ++i) {
        auto const original_obj = recursive_obj;
        recursive_array.emplace_back(recursive_obj);
        recursive_obj.emplace("obj_" + std::to_string(i), original_obj);
        recursive_obj.emplace("array_" + std::to_string(i), recursive_array);
        expected_auto_gen_and_user_gen_object_pairs.emplace_back(original_obj, recursive_obj);
        expected_auto_gen_and_user_gen_object_pairs.emplace_back(empty_obj, recursive_obj);
    }

    for (auto const& [auto_gen_json_obj, user_gen_json_obj] :
         expected_auto_gen_and_user_gen_object_pairs)
    {
        REQUIRE(unpack_and_serialize_msgpack_bytes(
                nlohmann::json::to_msgpack(auto_gen_json_obj),
                nlohmann::json::to_msgpack(user_gen_json_obj),
                serializer
        ));
    }
    flush_and_clear_serializer_buffer(serializer, ir_buf);
    ir_buf.push_back(clp::ffi::ir_stream::cProtocol::Eof);

    // Deserialize the results
    BufferReader reader{size_checked_pointer_cast<char>(ir_buf.data()), ir_buf.size()};
    auto deserializer_result{Deserializer<IrUnitHandler>::create(reader, IrUnitHandler{})};
    REQUIRE_FALSE(deserializer_result.has_error());
    auto& deserializer = deserializer_result.value();

    auto const& deserialized_metadata = deserializer.get_metadata();
    string const user_defined_metadata_key{
            clp::ffi::ir_stream::cProtocol::Metadata::UserDefinedMetadataKey
    };
    REQUIRE(deserialized_metadata.contains(user_defined_metadata_key));
    REQUIRE((deserialized_metadata.at(user_defined_metadata_key) == user_defined_metadata));

    while (true) {
        auto const result{deserializer.deserialize_next_ir_unit(reader)};
        REQUIRE_FALSE(result.has_error());
        if (result.value() == clp::ffi::ir_stream::IrUnitType::EndOfStream) {
            break;
        }
    }
    auto const& ir_unit_handler{deserializer.get_ir_unit_handler()};

    // Check the stream is complete
    REQUIRE(ir_unit_handler.is_complete());
    REQUIRE(deserializer.is_stream_completed());
    // Check the number of log events deserialized matches the number of log events serialized
    auto const& deserialized_log_events{ir_unit_handler.get_deserialized_log_events()};
    REQUIRE((expected_auto_gen_and_user_gen_object_pairs.size() == deserialized_log_events.size()));

    auto const num_log_events{expected_auto_gen_and_user_gen_object_pairs.size()};
    for (size_t idx{0}; idx < num_log_events; ++idx) {
        auto const& [expected_auto_gen_json_obj, expected_user_gen_json_obj]{
                expected_auto_gen_and_user_gen_object_pairs.at(idx)
        };
        auto const& deserialized_log_event{deserialized_log_events.at(idx)};

        auto const num_leaves_in_auto_gen_json_obj{count_num_leaves(expected_auto_gen_json_obj)};
        auto const num_auto_gen_kv_pairs{
                deserialized_log_event.get_auto_gen_node_id_value_pairs().size()
        };
        REQUIRE((num_leaves_in_auto_gen_json_obj == num_auto_gen_kv_pairs));

        auto const num_leaves_in_user_gen_json_obj{count_num_leaves(expected_user_gen_json_obj)};
        auto const num_user_gen_kv_pairs{
                deserialized_log_event.get_user_gen_node_id_value_pairs().size()
        };
        REQUIRE((num_leaves_in_user_gen_json_obj == num_user_gen_kv_pairs));

        auto const serialized_json_result{deserialized_log_event.serialize_to_json()};
        REQUIRE_FALSE(serialized_json_result.has_error());
        auto const& [actual_auto_gen_json_obj, actual_user_gen_json_obj]{
                serialized_json_result.value()
        };
        REQUIRE((expected_auto_gen_json_obj == actual_auto_gen_json_obj));
        REQUIRE((expected_user_gen_json_obj == actual_user_gen_json_obj));
    }

    auto const eof_result{deserializer.deserialize_next_ir_unit(reader)};
    REQUIRE((eof_result.has_error() && std::errc::operation_not_permitted == eof_result.error()));
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEMPLATE_TEST_CASE(
        "ffi_ir_stream_serialize_schema_tree_node_id",
        "[clp][ffi][ir_stream]",
        std::true_type,
        std::false_type
) {
    constexpr bool cIsAutoGeneratedNode{TestType{}};
    constexpr int8_t cOneByteLengthIndicatorTag{
            clp::ffi::ir_stream::cProtocol::Payload::EncodedSchemaTreeNodeIdByte
    };
    constexpr int8_t cTwoByteLengthIndicatorTag{
            clp::ffi::ir_stream::cProtocol::Payload::EncodedSchemaTreeNodeIdShort
    };
    constexpr int8_t cFourByteLengthIndicatorTag{
            clp::ffi::ir_stream::cProtocol::Payload::EncodedSchemaTreeNodeIdInt
    };
    constexpr auto cMaxNodeId{static_cast<clp::ffi::SchemaTree::Node::id_t>(INT32_MAX)};

    constexpr auto cSerializationMethodToTest
            = clp::ffi::ir_stream::encode_and_serialize_schema_tree_node_id<
                    cIsAutoGeneratedNode,
                    cOneByteLengthIndicatorTag,
                    cTwoByteLengthIndicatorTag,
                    cFourByteLengthIndicatorTag>;
    constexpr auto cDeserializationMethodToTest
            = clp::ffi::ir_stream::deserialize_and_decode_schema_tree_node_id<
                    cOneByteLengthIndicatorTag,
                    cTwoByteLengthIndicatorTag,
                    cFourByteLengthIndicatorTag>;

    std::vector<int8_t> output_buf;
    std::unordered_set<clp::ffi::SchemaTree::Node::id_t> valid_node_ids_to_test;

    // Add some boundary node IDs
    valid_node_ids_to_test.emplace(0);
    valid_node_ids_to_test.emplace(static_cast<clp::ffi::SchemaTree::Node::id_t>(INT8_MAX - 1));
    valid_node_ids_to_test.emplace(static_cast<clp::ffi::SchemaTree::Node::id_t>(INT8_MAX));
    valid_node_ids_to_test.emplace(static_cast<clp::ffi::SchemaTree::Node::id_t>(INT8_MAX + 1));
    valid_node_ids_to_test.emplace(static_cast<clp::ffi::SchemaTree::Node::id_t>(INT16_MAX - 1));
    valid_node_ids_to_test.emplace(static_cast<clp::ffi::SchemaTree::Node::id_t>(INT16_MAX));
    valid_node_ids_to_test.emplace(static_cast<clp::ffi::SchemaTree::Node::id_t>(INT16_MAX + 1));
    valid_node_ids_to_test.emplace(static_cast<clp::ffi::SchemaTree::Node::id_t>(INT32_MAX - 1));
    valid_node_ids_to_test.emplace(static_cast<clp::ffi::SchemaTree::Node::id_t>(INT32_MAX));

    // Generate some more "random" valid node IDs
    for (clp::ffi::SchemaTree::Node::id_t node_id{1}, step{1}; node_id <= cMaxNodeId;
         node_id += step, step += 1)
    {
        valid_node_ids_to_test.emplace(node_id);
    }

    for (auto const node_id : valid_node_ids_to_test) {
        output_buf.clear();
        REQUIRE(cSerializationMethodToTest(node_id, output_buf));

        BufferReader reader{size_checked_pointer_cast<char>(output_buf.data()), output_buf.size()};
        encoded_tag_t tag{};
        REQUIRE((IRErrorCode::IRErrorCode_Success == deserialize_tag(reader, tag)));
        auto const result{cDeserializationMethodToTest(tag, reader)};
        REQUIRE_FALSE(result.has_error());
        auto const [is_auto_generated, deserialized_node_id]{result.value()};
        REQUIRE((cIsAutoGeneratedNode == is_auto_generated));
        REQUIRE((deserialized_node_id == node_id));
    }

    // Test against the first invalid node ID
    REQUIRE_FALSE(cSerializationMethodToTest(
            static_cast<clp::ffi::SchemaTree::Node::id_t>(INT32_MAX) + 1,
            output_buf
    ));
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEMPLATE_TEST_CASE(
        "ffi_ir_stream_Serializer_serialize_invalid_msgpack",
        "[clp][ffi][ir_stream][Serializer]",
        four_byte_encoded_variable_t,
        eight_byte_encoded_variable_t
) {
    auto result{Serializer<TestType>::create()};
    REQUIRE((false == result.has_error()));

    std::stringstream msgpack_serialization_buffer;
    auto& serializer{result.value()};
    serializer.clear_ir_buf();

    auto assert_invalid_serialization = [&]<typename T>(T invalid_value) -> bool {
        std::map<string, T> const invalid_map{{"valid_key", invalid_value}};
        msgpack::pack(msgpack_serialization_buffer, invalid_map);
        return unpack_and_assert_serialization_failure(msgpack_serialization_buffer, serializer);
    };

    std::map<int, int> const map_with_integer_keys{{0, 0}, {1, 1}, {2, 2}};
    REQUIRE(assert_invalid_serialization(map_with_integer_keys));

    std::map<string, decltype(map_with_integer_keys)> const map_with_invalid_submap{
            {"valid_key", map_with_integer_keys}
    };
    REQUIRE(assert_invalid_serialization(map_with_invalid_submap));

    std::tuple<int, vector<uint8_t>> const array_with_invalid_type{0, {0x00, 0x00, 0x00}};
    REQUIRE(assert_invalid_serialization(array_with_invalid_type));

    std::tuple<int, decltype(array_with_invalid_type)> const subarray_with_invalid_type{
            0,
            array_with_invalid_type
    };
    REQUIRE(assert_invalid_serialization(subarray_with_invalid_type));

    std::tuple<int, decltype(map_with_integer_keys)> const array_with_invalid_map{
            0,
            map_with_integer_keys
    };
    REQUIRE(assert_invalid_serialization(array_with_invalid_map));

    std::tuple<int, decltype(array_with_invalid_map)> const subarray_with_invalid_map{
            0,
            array_with_invalid_map
    };
    REQUIRE(assert_invalid_serialization(subarray_with_invalid_map));

    std::tuple<int, decltype(map_with_invalid_submap)> const array_with_invalid_submap{
            0,
            map_with_invalid_submap
    };
    REQUIRE(assert_invalid_serialization(array_with_invalid_submap));
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEMPLATE_TEST_CASE(
        "ffi_ir_stream_Serializer_serialize_invalid_user_defined_metadata",
        "[clp][ffi][ir_stream][Serializer]",
        four_byte_encoded_variable_t,
        eight_byte_encoded_variable_t
) {
    auto invalid_user_defined_metadata = GENERATE(
            nlohmann::json(std::string{"str"}),
            nlohmann::json(int{0}),
            nlohmann::json(double{0.0}),
            nlohmann::json(true),
            nlohmann::json(nullptr),
            nlohmann::json(vector<int>{0, 1, 2})
    );
    auto const serializer_result{Serializer<TestType>::create(invalid_user_defined_metadata)};
    REQUIRE(serializer_result.has_error());
    REQUIRE((std::errc::protocol_not_supported == serializer_result.error()));
}
