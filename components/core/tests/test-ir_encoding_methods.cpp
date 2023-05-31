// Catch2
#include "../submodules/Catch2/single_include/catch2/catch.hpp"

// json
#include "../submodules/json/single_include/nlohmann/json.hpp"

// Project headers
#include "../src/ffi/encoding_methods.hpp"
#include "../src/ffi/ir_stream/encoding_methods.hpp"
#include "../src/ffi/ir_stream/decoding_methods.hpp"
#include "../src/ffi/ir_stream/protocol_constants.hpp"
#include "../src/BufferReader.hpp"

using ffi::decode_float_var;
using ffi::decode_integer_var;
using ffi::decode_message;
using ffi::eight_byte_encoded_variable_t;
using ffi::encode_float_string;
using ffi::encode_integer_string;
using ffi::encode_message;
using ffi::epoch_time_ms_t;
using ffi::four_byte_encoded_variable_t;
using ffi::get_bounds_of_next_var;
using ffi::ir_stream::cProtocol::EightByteEncodingMagicNumber;
using ffi::ir_stream::cProtocol::FourByteEncodingMagicNumber;
using ffi::ir_stream::cProtocol::MagicNumberLength;
using ffi::ir_stream::get_encoding_type;
using ffi::ir_stream::decode_preamble;
using ffi::ir_stream::IRErrorCode;
using ffi::ir_stream::encoded_tag_t;
using ffi::VariablePlaceholder;
using ffi::wildcard_query_matches_any_encoded_var;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;
using std::is_same_v;
using std::string_view;
using std::string;
using std::vector;

static epoch_time_ms_t get_current_ts ();

/**
 * @tparam encoded_variable_t Type of the encoded variable
 * @param is_four_bytes_encoding
 * @return True if input encoding type matches the type of encoded_variable_t
 * false otherwise
 */
template <typename encoded_variable_t>
bool match_encoding_type (bool is_four_bytes_encoding);

template <typename encoded_variable_t>
epoch_time_ms_t get_next_timestamp_for_test ();

/**
 * Helper function that encodes a preamble of encoding type = encoded_variable_t
 * and writes into ir_buf
 * @tparam encoded_variable_t Type of the encoded variable
 * @param timestamp_pattern
 * @param timestamp_pattern_syntax
 * @param time_zone_id
 * @param reference_timestamp Only used
 * when encoded_variable_t == four_byte_encoded_variable_t
 * @param ir_buf
 * @return True if preamble is encoded without error, otherwise false
 */
template <typename encoded_variable_t>
bool encode_preamble (string_view timestamp_pattern,
                      string_view timestamp_pattern_syntax, string_view time_zone_id,
                      epoch_time_ms_t reference_timestamp, vector<int8_t>& ir_buf);

/**
 * Helper function that encodes a message of encoding type = encoded_variable_t
 * and writes into ir_buf
 * @tparam encoded_variable_t Type of the encoded variable
 * @param timestamp
 * @param message
 * @param logtype
 * @param ir_buf
 * @return True if message is encoded without error, otherwise false
 */
template <typename encoded_variable_t>
bool encode_message (epoch_time_ms_t timestamp, string_view message, string& logtype,
                     vector<int8_t>& ir_buf);

/**
 * Helper function that decodes a message of encoding type = encoded_variable_t
 * from the ir_buf
 * @tparam encoded_variable_t Type of the encoded variable
 * @param ir_buf
 * @param message
 * @param decoded_ts Returns the decoded timestamp
 * @return IRErrorCode_Success on success, otherwise
 * Same as the ffi::ir_stream::eight_byte_encoding::decode_next_message when
 * encoded_variable_t == eight_byte_encoded_variable_t
 * Same as the ffi::ir_stream::four_byte_encoding::decode_next_message when
 * encoded_variable_t == four_byte_encoded_variable_t
 */
template <typename encoded_variable_t>
IRErrorCode decode_next_message (BufferReader& ir_buf, string& message, epoch_time_ms_t& decoded_ts);

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
static void set_timestamp_info (const nlohmann::json& metadata_json, TimestampInfo& ts_info);

static epoch_time_ms_t get_current_ts () {
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

template <typename encoded_variable_t>
bool match_encoding_type (bool is_four_bytes_encoding) {
    static_assert(is_same_v<encoded_variable_t, eight_byte_encoded_variable_t> ||
                  is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);

    if constexpr (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        return false == is_four_bytes_encoding;
    } else {
        return is_four_bytes_encoding;
    }
}

template <typename encoded_variable_t>
epoch_time_ms_t get_next_timestamp_for_test () {
    static_assert(is_same_v<encoded_variable_t, eight_byte_encoded_variable_t> ||
                  is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);

    // We return an absolute timestamp for the eight-byte encoding and a mocked
    // timestamp delta for the four-byte encoding
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
bool encode_preamble (string_view timestamp_pattern,
                      string_view timestamp_pattern_syntax, string_view time_zone_id,
                      epoch_time_ms_t reference_timestamp, vector<int8_t>& ir_buf) {
    static_assert(is_same_v<encoded_variable_t, eight_byte_encoded_variable_t> ||
                  is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);

    if constexpr (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        return ffi::ir_stream::eight_byte_encoding::encode_preamble(timestamp_pattern,
                                                                    timestamp_pattern_syntax,
                                                                    time_zone_id, ir_buf);
    } else {
        return ffi::ir_stream::four_byte_encoding::encode_preamble(timestamp_pattern,
                                                                   timestamp_pattern_syntax,
                                                                   time_zone_id,
                                                                   reference_timestamp, ir_buf);
    }
}

template <typename encoded_variable_t>
bool encode_message (epoch_time_ms_t timestamp, string_view message, string& logtype,
                     vector<int8_t>& ir_buf) {
    static_assert(is_same_v<encoded_variable_t, eight_byte_encoded_variable_t> ||
                  is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);

    if constexpr (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        return ffi::ir_stream::eight_byte_encoding::encode_message(timestamp, message, logtype,
                                                                   ir_buf);
    } else {
        return ffi::ir_stream::four_byte_encoding::encode_message(timestamp, message, logtype,
                                                                  ir_buf);
    }
}

template <typename encoded_variable_t>
IRErrorCode decode_next_message (BufferReader& ir_buf, string& message, epoch_time_ms_t& decoded_ts) {
    static_assert(is_same_v<encoded_variable_t, eight_byte_encoded_variable_t> ||
                  is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);

    if constexpr (is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        return ffi::ir_stream::eight_byte_encoding::decode_next_message(ir_buf, message,
                                                                        decoded_ts);
    } else {
        return ffi::ir_stream::four_byte_encoding::decode_next_message(ir_buf, message, decoded_ts);
    }
}

static void set_timestamp_info (const nlohmann::json& metadata_json, TimestampInfo& ts_info) {
    ts_info.time_zone_id = metadata_json.at(ffi::ir_stream::cProtocol::Metadata::TimeZoneIdKey);
    ts_info.timestamp_pattern =
        metadata_json.at(ffi::ir_stream::cProtocol::Metadata::TimestampPatternKey);
    ts_info.timestamp_pattern_syntax =
        metadata_json.at(ffi::ir_stream::cProtocol::Metadata::TimestampPatternSyntaxKey);
}

TEST_CASE("get_encoding_type", "[ffi][get_encoding_type]") {
    bool is_four_bytes_encoding;
    // Test eight-byte encoding
    vector<int8_t> eight_byte_encoding_vec{EightByteEncodingMagicNumber,
                                           EightByteEncodingMagicNumber + MagicNumberLength};

    // Test eight-byte encoding
    BufferReader eight_byte_encoding_buffer (
            reinterpret_cast<const char*>(eight_byte_encoding_vec.data()),
            eight_byte_encoding_vec.size()
    );
    REQUIRE(get_encoding_type(eight_byte_encoding_buffer, is_four_bytes_encoding) ==
            IRErrorCode::IRErrorCode_Success);
    REQUIRE(match_encoding_type<eight_byte_encoded_variable_t>(is_four_bytes_encoding));

    // Test four-byte encoding
    vector<int8_t> four_byte_encoding_vec{FourByteEncodingMagicNumber,
                                          FourByteEncodingMagicNumber + MagicNumberLength};

    BufferReader four_byte_encoding_buffer (
            reinterpret_cast<const char*>(four_byte_encoding_vec.data()),
             four_byte_encoding_vec.size()
    );
    REQUIRE(get_encoding_type(four_byte_encoding_buffer, is_four_bytes_encoding) ==
            IRErrorCode::IRErrorCode_Success);
    REQUIRE(match_encoding_type<four_byte_encoded_variable_t>(is_four_bytes_encoding));

    // Test error on incomplete ir_buffer
    BufferReader incomplete_buffer (
            reinterpret_cast<const char*>(four_byte_encoding_vec.data()),
            four_byte_encoding_vec.size() - 1
    );

    REQUIRE(get_encoding_type(incomplete_buffer, is_four_bytes_encoding) ==
            IRErrorCode::IRErrorCode_Incomplete_IR);

    // Test error on invalid encoding
    const vector<int8_t> invalid_ir_vec{0x02, 0x43, 0x24, 0x34};
    BufferReader invalid_ir_buffer (
            reinterpret_cast<const char*>(invalid_ir_vec.data()),
            invalid_ir_vec.size()
    );
    REQUIRE(get_encoding_type(invalid_ir_buffer, is_four_bytes_encoding) ==
            IRErrorCode::IRErrorCode_Corrupted_IR);

}

TEMPLATE_TEST_CASE("decode_preamble", "[ffi][decode_preamble]", four_byte_encoded_variable_t,
                   eight_byte_encoded_variable_t)
{
    vector<int8_t> ir_buf;
    constexpr char timestamp_pattern[] = "%Y-%m-%d %H:%M:%S,%3";
    constexpr char timestamp_pattern_syntax[] = "yyyy-MM-dd HH:mm:ss";
    constexpr char time_zone_id[] = "Asia/Tokyo";
    const epoch_time_ms_t reference_ts = get_current_ts();
    REQUIRE(encode_preamble<TestType>(timestamp_pattern, timestamp_pattern_syntax, time_zone_id,
                                      reference_ts, ir_buf));
    const size_t encoded_preamble_end_pos = ir_buf.size();

    // Check if encoding type is properly read
    BufferReader encoding_buffer (
            reinterpret_cast<const char*>(ir_buf.data()), ir_buf.size()
    );
    bool is_four_bytes_encoding;
    REQUIRE(get_encoding_type(encoding_buffer, is_four_bytes_encoding) ==
            IRErrorCode::IRErrorCode_Success);
    REQUIRE(match_encoding_type<TestType>(is_four_bytes_encoding));
    REQUIRE(MagicNumberLength == encoding_buffer.get_pos());

    // Test if preamble can be decoded correctly
    TimestampInfo ts_info;
    encoded_tag_t metadata_type{0};
    size_t metadata_pos{0};
    uint16_t metadata_size{0};
    REQUIRE(decode_preamble(encoding_buffer, metadata_type, metadata_pos, metadata_size) ==
            IRErrorCode::IRErrorCode_Success);
    REQUIRE(encoded_preamble_end_pos == encoding_buffer.get_pos());
    
    auto json_metadata_ptr = reinterpret_cast<char*>(ir_buf.data() + metadata_pos);
    string_view json_metadata_ref {json_metadata_ptr, metadata_size};

    // Test if preamble can be decoded by the string copy method
    std::vector<int8_t> json_metadata_vec;
    encoding_buffer.seek_from_begin(MagicNumberLength);
    REQUIRE(decode_preamble(encoding_buffer, metadata_type, json_metadata_vec) ==
            IRErrorCode::IRErrorCode_Success);
    string_view json_metadata_copied { reinterpret_cast<const char*>(json_metadata_vec.data()),
                                       json_metadata_vec.size() };
    REQUIRE (json_metadata_copied == json_metadata_ref);

    auto metadata_json = nlohmann::json::parse(json_metadata_ref);
    REQUIRE(ffi::ir_stream::cProtocol::Metadata::VersionValue ==
            metadata_json.at(ffi::ir_stream::cProtocol::Metadata::VersionKey));
    REQUIRE(ffi::ir_stream::cProtocol::Metadata::EncodingJson == metadata_type);
    set_timestamp_info(metadata_json, ts_info);
    REQUIRE(timestamp_pattern_syntax == ts_info.timestamp_pattern_syntax);
    REQUIRE(time_zone_id == ts_info.time_zone_id);
    REQUIRE(timestamp_pattern == ts_info.timestamp_pattern);
    REQUIRE(encoded_preamble_end_pos == encoding_buffer.get_pos());

    if constexpr (is_same_v<TestType, four_byte_encoded_variable_t>) {
        REQUIRE(reference_ts ==
                std::stoll(
                        metadata_json.at(ffi::ir_stream::cProtocol::Metadata::ReferenceTimestampKey)
                                .get<string>()));
    }

    // Test if incomplete IR can be detected
    ir_buf.resize(encoded_preamble_end_pos - 1);
    BufferReader incomplete_preamble_buffer(reinterpret_cast<const char*>(ir_buf.data()),
                                            ir_buf.size());
    incomplete_preamble_buffer.seek_from_begin(MagicNumberLength);
    REQUIRE(decode_preamble(
                    incomplete_preamble_buffer, metadata_type, metadata_pos, metadata_size) ==
            IRErrorCode::IRErrorCode_Incomplete_IR);

    // Test if corrupted IR can be detected
    ir_buf[MagicNumberLength] = 0x23;
    BufferReader corrupted_preamble_buffer(reinterpret_cast<const char*>(ir_buf.data()),
                                           ir_buf.size());
    REQUIRE(decode_preamble(
                    corrupted_preamble_buffer, metadata_type, metadata_pos, metadata_size) ==
            IRErrorCode::IRErrorCode_Corrupted_IR);
}

TEMPLATE_TEST_CASE("decode_next_message_general", "[ffi][decode_next_message]",
                   four_byte_encoded_variable_t, eight_byte_encoded_variable_t)
{
    vector<int8_t> ir_buf;
    string logtype;

    string placeholder_as_string{enum_to_underlying_type(VariablePlaceholder::Dictionary)};
    string message = "Static <\text>, dictVar1, 123, 456.7 dictVar2, 987, 654.3," +
                     placeholder_as_string + " end of static text";
    epoch_time_ms_t reference_timestamp = get_next_timestamp_for_test<TestType>();
    REQUIRE(true == encode_message<TestType>(reference_timestamp, message, logtype, ir_buf));
    const size_t encoded_message_end_pos = ir_buf.size();
    const size_t encoded_message_start_pos = 0;

    // Test if message can be decoded properly

    BufferReader ir_buffer(reinterpret_cast<const char*>(ir_buf.data()), ir_buf.size());
    string decoded_message;
    epoch_time_ms_t timestamp;

    REQUIRE(IRErrorCode::IRErrorCode_Success ==
            decode_next_message<TestType>(ir_buffer, decoded_message, timestamp));
    REQUIRE(message == decoded_message);
    REQUIRE(timestamp == reference_timestamp);
    REQUIRE(ir_buffer.get_pos() == encoded_message_end_pos);

    // Test corrupted IR
    ir_buffer.seek_from_begin(encoded_message_start_pos + 1);
    REQUIRE(IRErrorCode::IRErrorCode_Corrupted_IR ==
            decode_next_message<TestType>(ir_buffer, message, timestamp));

    // Test incomplete IR
    ir_buf.resize(encoded_message_end_pos - 4);
    ir_buffer.set_buffer(reinterpret_cast<const char*>(ir_buf.data()), ir_buf.size());
    REQUIRE(IRErrorCode::IRErrorCode_Incomplete_IR ==
            decode_next_message<TestType>(ir_buffer, message, timestamp));
}

// NOTE: This test only tests eight_byte_encoded_variable_t because we trigger
// IRErrorCode_Decode_Error by manually modifying the logtype within the IR, and
// this is easier for the eight_byte_encoded_variable_t case.
TEST_CASE("message_decode_error", "[ffi][decode_next_message]")
{
    vector<int8_t> ir_buf;
    string logtype;

    string placeholder_as_string{enum_to_underlying_type(VariablePlaceholder::Dictionary)};
    string message = "Static <\text>, dictVar1, 123, 456.7 dictVar2, 987, 654.3," +
                     placeholder_as_string + " end of static text";
    epoch_time_ms_t reference_ts = get_next_timestamp_for_test<eight_byte_encoded_variable_t>();
    REQUIRE(true == encode_message<eight_byte_encoded_variable_t>(reference_ts, message,
                                                                  logtype, ir_buf));

    // Find the end of the encoded logtype which is before the encoded timestamp
    // The timestamp is encoded as tagbyte + eight_byte_encoded_variable_t
    size_t timestamp_encoding_size = sizeof(ffi::ir_stream::cProtocol::Payload::TimestampVal) +
                                     sizeof(eight_byte_encoded_variable_t);
    const size_t logtype_end_pos = ir_buf.size() - timestamp_encoding_size;

    string decoded_message;
    epoch_time_ms_t timestamp;

    // Test if a trailing escape triggers a decoder error
    auto ir_with_extra_escape {ir_buf};
    ir_with_extra_escape.at(logtype_end_pos - 1) = ffi::cVariablePlaceholderEscapeCharacter;
    BufferReader ir_with_extra_escape_buffer (
            reinterpret_cast<const char*>(ir_with_extra_escape.data()),
            ir_with_extra_escape.size()
    );
    REQUIRE(IRErrorCode::IRErrorCode_Decode_Error ==
            decode_next_message<eight_byte_encoded_variable_t>(ir_with_extra_escape_buffer,
                                                               decoded_message,
                                                               timestamp));

    // Test if an extra placeholder triggers a decoder error
    auto ir_with_extra_placeholder{ir_buf};
    ir_with_extra_placeholder.at(logtype_end_pos - 1) =
            enum_to_underlying_type(VariablePlaceholder::Dictionary);
    BufferReader ir_with_extra_placeholder_buffer (
            reinterpret_cast<const char*>(ir_with_extra_placeholder.data()),
            ir_with_extra_placeholder.size()
    );
    REQUIRE(IRErrorCode::IRErrorCode_Decode_Error ==
            decode_next_message<eight_byte_encoded_variable_t>(ir_with_extra_placeholder_buffer,
                                                               decoded_message,
                                                               timestamp));
}

TEST_CASE("decode_next_message_four_byte_negative_delta", "[ffi][decode_next_message]") {
    string message = "Static <\text>, dictVar1, 123, 456345232.7234223, "
                     "dictVar2, 987, 654.3, end of static text";
    vector<int8_t> ir_buf;
    string logtype;

    epoch_time_ms_t reference_delta_ts_negative = -5;
    REQUIRE(true == encode_message<four_byte_encoded_variable_t>(reference_delta_ts_negative,
                                                                 message, logtype, ir_buf));

    BufferReader ir_buffer(reinterpret_cast<const char*>(ir_buf.data()), ir_buf.size());
    string decoded_message;
    epoch_time_ms_t delta_ts;
    REQUIRE(IRErrorCode::IRErrorCode_Success ==
            decode_next_message<four_byte_encoded_variable_t>(ir_buffer, decoded_message,
                                                              delta_ts));
    REQUIRE(message == decoded_message);
    REQUIRE(delta_ts == reference_delta_ts_negative);
}

TEMPLATE_TEST_CASE("decode_ir_complete", "[ffi][decode_next_message]",
                   four_byte_encoded_variable_t, eight_byte_encoded_variable_t) {
    vector<int8_t> ir_buf;
    string logtype;

    epoch_time_ms_t preamble_ts = get_current_ts();
    constexpr char timestamp_pattern[] = "%Y-%m-%d %H:%M:%S,%3";
    constexpr char timestamp_pattern_syntax[] = "yyyy-MM-dd HH:mm:ss";
    constexpr char time_zone_id[] = "Asia/Tokyo";
    REQUIRE(encode_preamble<TestType>(timestamp_pattern, timestamp_pattern_syntax, time_zone_id,
                                      preamble_ts, ir_buf));
    const size_t encoded_preamble_end_pos = ir_buf.size();

    string message;
    epoch_time_ms_t ts;
    vector<string> reference_messages;
    vector<epoch_time_ms_t> reference_timestamps;

    // First message:
    message = "Static <\text>, dictVar1, 123, 456.7, dictVar2, 987, 654.3, end of static text";
    ts = get_next_timestamp_for_test<TestType>();
    REQUIRE(encode_message<TestType>(ts, message, logtype, ir_buf));
    reference_messages.push_back(message);
    reference_timestamps.push_back(ts);

    // Second message:
    message = "Static <\text>, dictVar3, 355.2352512, 23953324532112, "
              "python3.4.6, end of static text";
    ts = get_next_timestamp_for_test<TestType>();
    REQUIRE(encode_message<TestType>(ts, message, logtype, ir_buf));
    reference_messages.push_back(message);
    reference_timestamps.push_back(ts);

    BufferReader complete_encoding_buffer(reinterpret_cast<const char*>(ir_buf.data()),
                                          ir_buf.size());

    bool is_four_bytes_encoding;
    REQUIRE(get_encoding_type(complete_encoding_buffer, is_four_bytes_encoding) ==
            IRErrorCode::IRErrorCode_Success);
    REQUIRE(match_encoding_type<TestType>(is_four_bytes_encoding));

    // Test if preamble can be properly decoded
    TimestampInfo ts_info;
    encoded_tag_t metadata_type;
    size_t metadata_pos;
    uint16_t metadata_size;
    REQUIRE(decode_preamble(complete_encoding_buffer, metadata_type, metadata_pos, metadata_size) ==
            IRErrorCode::IRErrorCode_Success);
    REQUIRE(encoded_preamble_end_pos == complete_encoding_buffer.get_pos());

    auto json_metadata_ptr = reinterpret_cast<char*>(ir_buf.data() + metadata_pos);
    string_view json_metadata {json_metadata_ptr, metadata_size};
    auto metadata_json = nlohmann::json::parse(json_metadata);
    REQUIRE(ffi::ir_stream::cProtocol::Metadata::VersionValue ==
            metadata_json.at(ffi::ir_stream::cProtocol::Metadata::VersionKey));
    REQUIRE(ffi::ir_stream::cProtocol::Metadata::EncodingJson == metadata_type);
    set_timestamp_info(metadata_json, ts_info);
    REQUIRE(timestamp_pattern_syntax == ts_info.timestamp_pattern_syntax);
    REQUIRE(time_zone_id == ts_info.time_zone_id);
    REQUIRE(timestamp_pattern == ts_info.timestamp_pattern);

    string decoded_message;
    epoch_time_ms_t timestamp;
    for (size_t ix = 0; ix < reference_messages.size(); ix++) {
        REQUIRE(IRErrorCode::IRErrorCode_Success ==
                decode_next_message<TestType>(complete_encoding_buffer, decoded_message,
                                              timestamp));
        REQUIRE(decoded_message == reference_messages[ix]);
        REQUIRE(timestamp == reference_timestamps[ix]);
    }
    REQUIRE(complete_encoding_buffer.get_pos() == ir_buf.size());
}
