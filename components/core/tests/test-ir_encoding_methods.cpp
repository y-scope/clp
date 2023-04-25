// Catch2
#include "../submodules/Catch2/single_include/catch2/catch.hpp"

// Project headers
#include "../src/ffi/encoding_methods.hpp"
#include "../src/ffi/ir_stream/encoding_methods.hpp"
#include "../src/ffi/ir_stream/decoding_methods.hpp"
#include "../src/ffi/ir_stream/protocol_constants.hpp"

using ffi::decode_float_var;
using ffi::decode_integer_var;
using ffi::decode_message;
using ffi::eight_byte_encoded_variable_t;
using ffi::four_byte_encoded_variable_t;
using ffi::encode_float_string;
using ffi::encode_integer_string;
using ffi::encode_message;
using ffi::get_bounds_of_next_var;
using ffi::VariablePlaceholder;
using ffi::wildcard_query_matches_any_encoded_var;
using std::string;
using std::vector;
using ffi::ir_stream::IRErrorCode;
using std::chrono::milliseconds;
using std::chrono::system_clock;

static ffi::epoch_time_ms_t get_current_ts () {
    return std::chrono::duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()).count();
}

template <typename encoded_variable_t>
bool match_encoding_type (bool is_four_bytes_encoding) {
    static_assert(std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t> ||
                  std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);

    if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        return false == is_four_bytes_encoding;
    } else {
        return is_four_bytes_encoding;
    }
}

template <typename encoded_variable_t>
ffi::epoch_time_ms_t get_next_timestamp_for_test () {
    static_assert(std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t> ||
                  std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);

    // we return complete timestamp for 8-bytes encoding and a faked-up delta_ts for 4-bytes encoding
    if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        return get_current_ts();
    } else {
        ffi::epoch_time_ms_t ts1 = get_current_ts();
        ffi::epoch_time_ms_t ts2 = get_current_ts();
        return ts2 - ts1;
    }
}

// A helper function to generalize the testing caller interface.
// The reference_timestamp is only used by four bytes encoding
template <typename encoded_variable_t>
bool encode_preamble (std::string_view timestamp_pattern,
                      std::string_view timestamp_pattern_syntax, std::string_view time_zone_id,
                      ffi::epoch_time_ms_t reference_timestamp, std::vector<int8_t>& ir_buf)
{
    static_assert(std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t> ||
                  std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);

    if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
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
IRErrorCode decode_preamble (ffi::ir_stream::IRBuffer& ir_buf,
                             ffi::ir_stream::TimestampInfo& ts_info,
                             ffi::epoch_time_ms_t& reference_ts) {
    static_assert(std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t> ||
                  std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);

    if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        return ffi::ir_stream::eight_byte_encoding::decode_preamble(ir_buf, ts_info);
    } else {
        return ffi::ir_stream::four_byte_encoding::decode_preamble(ir_buf, ts_info, reference_ts);
    }
}

template <typename encoded_variable_t>
bool encode_message (ffi::epoch_time_ms_t timestamp, std::string_view message, string& logtype,
                     vector<int8_t>& ir_buf) {
    static_assert(std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t> ||
                  std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);

    if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        return ffi::ir_stream::eight_byte_encoding::encode_message(timestamp, message, logtype,
                                                                   ir_buf);
    } else {
        return ffi::ir_stream::four_byte_encoding::encode_message(timestamp, message, logtype,
                                                                  ir_buf);
    }
}

template <typename encoded_variable_t>
IRErrorCode decode_next_message (ffi::ir_stream::IRBuffer& ir_buf,
                                 std::string& message,
                                 ffi::epoch_time_ms_t& decoded_ts) {
    static_assert(std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t> ||
                  std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);

    if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        return ffi::ir_stream::eight_byte_encoding::decode_next_message(ir_buf, message,
                                                                        decoded_ts);
    } else {
        return ffi::ir_stream::four_byte_encoding::decode_next_message(ir_buf, message,
                                                                       decoded_ts);
    }
}

TEST_CASE("check_encoding_type", "[ffi][check_encoding_type]") {
    bool is_four_bytes_encoding;

    // Test eight-byte encoding
    std::vector<int8_t> eight_byte_encoding_vec;
    eight_byte_encoding_vec.resize(ffi::ir_stream::cProtocol::MagicNumberLength);
    memcpy(eight_byte_encoding_vec.data(), ffi::ir_stream::cProtocol::EightByteEncodingMagicNumber,
           ffi::ir_stream::cProtocol::MagicNumberLength);

    ffi::ir_stream::IRBuffer eight_byte_ir_buffer(eight_byte_encoding_vec.data(),
                                                  eight_byte_encoding_vec.size());
    REQUIRE(ffi::ir_stream::get_encoding_type(eight_byte_ir_buffer, is_four_bytes_encoding) ==
            IRErrorCode::IRErrorCode_Success);
    REQUIRE(match_encoding_type<eight_byte_encoded_variable_t>(is_four_bytes_encoding));

    // Test four-byte encoding
    std::vector<int8_t> four_byte_encoding_vec;
    four_byte_encoding_vec.resize(ffi::ir_stream::cProtocol::MagicNumberLength);
    memcpy(four_byte_encoding_vec.data(), ffi::ir_stream::cProtocol::FourByteEncodingMagicNumber,
           ffi::ir_stream::cProtocol::MagicNumberLength);

    ffi::ir_stream::IRBuffer four_byte_ir_buffer(four_byte_encoding_vec.data(),
                                                 four_byte_encoding_vec.size());
    REQUIRE(ffi::ir_stream::get_encoding_type(four_byte_ir_buffer, is_four_bytes_encoding) ==
            IRErrorCode::IRErrorCode_Success);
    REQUIRE(match_encoding_type<four_byte_encoded_variable_t>(is_four_bytes_encoding));

    // Test error on empty and incomplete ir_buffer
    const std::vector<int8_t> empty_ir_vec;
    ffi::ir_stream::IRBuffer empty_ir_buffer(empty_ir_vec.data(), empty_ir_vec.size());
    REQUIRE(ffi::ir_stream::get_encoding_type(empty_ir_buffer, is_four_bytes_encoding) ==
            IRErrorCode::IRErrorCode_Incomplete_IR);

    ffi::ir_stream::IRBuffer incomplete_ir_buffer(four_byte_encoding_vec.data(),
                                                  four_byte_encoding_vec.size() - 1);
    REQUIRE(ffi::ir_stream::get_encoding_type(incomplete_ir_buffer, is_four_bytes_encoding) ==
            IRErrorCode::IRErrorCode_Incomplete_IR);

    // Test error on invalid encoding.
    const std::vector<int8_t> invalid_ir_vec{0x02, 0x43, 0x24, 0x34};
    ffi::ir_stream::IRBuffer invalid_ir_buffer(invalid_ir_vec.data(), invalid_ir_vec.size());
    REQUIRE(ffi::ir_stream::get_encoding_type(invalid_ir_buffer, is_four_bytes_encoding) ==
            IRErrorCode::IRErrorCode_Corrupted_IR);

}

TEMPLATE_TEST_CASE("decode_preamble_general", "[ffi][decode_preamble]",
                   four_byte_encoded_variable_t, eight_byte_encoded_variable_t)
{
    string message = "Static text, dictVar1, 123, 456.7, dictVar2, 987, 654.3";

    std::vector<int8_t> ir_buf;
    const std::string timestamp_pattern = "%Y-%m-%d %H:%M:%S,%3";
    const std::string timestamp_pattern_syntax = "yyyy-MM-dd HH:mm:ss";
    const std::string time_zone_id = "Asia/Tokyo";
    const ffi::epoch_time_ms_t reference_ts = get_current_ts();
    REQUIRE(encode_preamble<TestType>(timestamp_pattern, timestamp_pattern_syntax, time_zone_id,
                                      reference_ts, ir_buf));
    const size_t encoded_preamble_end_pos = ir_buf.size();

    // Check if encoding type is properly read
    ffi::ir_stream::TimestampInfo ts_info;
    ffi::ir_stream::IRBuffer preamble_buffer(ir_buf.data(), ir_buf.size());
    ffi::epoch_time_ms_t decoded_ts;
    bool is_four_bytes_encoding;
    REQUIRE(ffi::ir_stream::get_encoding_type(preamble_buffer, is_four_bytes_encoding) ==
            IRErrorCode::IRErrorCode_Success);
    REQUIRE(match_encoding_type<TestType>(is_four_bytes_encoding));
    REQUIRE(ffi::ir_stream::cProtocol::MagicNumberLength == preamble_buffer.get_cursor_pos());

    // Test if preamble can be decoded correctly
    REQUIRE(decode_preamble<TestType>(preamble_buffer, ts_info, decoded_ts) ==
            IRErrorCode::IRErrorCode_Success);
    REQUIRE(timestamp_pattern_syntax == ts_info.timestamp_pattern_syntax);
    REQUIRE(time_zone_id == ts_info.time_zone_id);
    REQUIRE(timestamp_pattern == ts_info.timestamp_pattern);
    REQUIRE(encoded_preamble_end_pos == preamble_buffer.get_cursor_pos());
    if constexpr (std::is_same_v<TestType, four_byte_encoded_variable_t>) {
        REQUIRE(reference_ts == decoded_ts);
    }

    // Test if incomplete IR can be detected.
    ir_buf.resize(encoded_preamble_end_pos - 1);
    ffi::ir_stream::IRBuffer incomplete_preamble_buffer(ir_buf.data(), ir_buf.size());
    incomplete_preamble_buffer.set_cursor_pos(ffi::ir_stream::cProtocol::MagicNumberLength);
    REQUIRE(decode_preamble<TestType>(incomplete_preamble_buffer, ts_info, decoded_ts) ==
            IRErrorCode::IRErrorCode_Incomplete_IR);

    // Test if corrupted IR can be detected.
    ir_buf.at(ffi::ir_stream::cProtocol::MagicNumberLength) = 0x23;
    ffi::ir_stream::IRBuffer corrupted_preamble_buffer(ir_buf.data(), ir_buf.size());
    incomplete_preamble_buffer.set_cursor_pos(ffi::ir_stream::cProtocol::MagicNumberLength);
    REQUIRE(decode_preamble<TestType>(corrupted_preamble_buffer, ts_info, decoded_ts) ==
            IRErrorCode::IRErrorCode_Corrupted_IR);
}

TEMPLATE_TEST_CASE("decode_next_message_general", "[ffi][decode_next_message]",
                   four_byte_encoded_variable_t, eight_byte_encoded_variable_t) {

    string message = "Static <\text>, dictVar1, 123, 456.7, "
                     "dictVar2, 987, 654.3, end of static text";

    std::vector<int8_t> ir_buf;
    std::string logtype;
    ffi::epoch_time_ms_t reference_timestamp = get_next_timestamp_for_test<TestType>();
    REQUIRE(true == encode_message<TestType>(reference_timestamp, message, logtype, ir_buf));
    const size_t encoded_message_end_pos = ir_buf.size();
    const size_t encoded_message_start_pos = 0;

    string decoded_message;
    ffi::epoch_time_ms_t timestamp;
    ffi::ir_stream::IRBuffer encoded_message_buffer(ir_buf.data(), ir_buf.size());

    REQUIRE(ffi::ir_stream::IRErrorCode_Success ==
            decode_next_message<TestType>(encoded_message_buffer, decoded_message, timestamp));
    REQUIRE(message == decoded_message);
    REQUIRE(timestamp == reference_timestamp);
    REQUIRE(encoded_message_buffer.get_cursor_pos() == encoded_message_end_pos);

    encoded_message_buffer.set_cursor_pos(encoded_message_start_pos + 1);
    REQUIRE(ffi::ir_stream::IRErrorCode_Corrupted_IR ==
            decode_next_message<TestType>(encoded_message_buffer, message, timestamp));

    ir_buf.resize(encoded_message_end_pos - 4);
    ffi::ir_stream::IRBuffer incomplete_message_buffer(ir_buf.data(), ir_buf.size());
    REQUIRE(ffi::ir_stream::IRErrorCode_Incomplete_IR ==
            decode_next_message<TestType>(incomplete_message_buffer, message, timestamp));
}

// Corner case for 4-bytes encoding
TEST_CASE("decode_next_message-4bytes", "[ffi][decode_next_message]") {
    string message = "Static <\text>, dictVar1, 123, 456345232.7234223, "
                     "dictVar2, 987, 654.3, end of static text";
    std::vector<int8_t> ir_buf;
    std::string logtype;
    // ensure that decoder can handle negative ts_delta
    ffi::epoch_time_ms_t reference_delta_ts_negative = -5;
    REQUIRE(true ==
            encode_message<four_byte_encoded_variable_t>(reference_delta_ts_negative, message,
                                                         logtype, ir_buf));

    ffi::ir_stream::IRBuffer encoded_message_buffer(ir_buf.data(), ir_buf.size());
    string decoded_message;
    ffi::epoch_time_ms_t delta_ts;
    REQUIRE(ffi::ir_stream::IRErrorCode_Success ==
            decode_next_message<four_byte_encoded_variable_t>(encoded_message_buffer,
                                                              decoded_message, delta_ts));
    REQUIRE(message == decoded_message);
    REQUIRE(delta_ts == reference_delta_ts_negative);
}

TEMPLATE_TEST_CASE("decode_ir_complete", "[ffi][decode_next_message]",
                   four_byte_encoded_variable_t, eight_byte_encoded_variable_t) {
    string message;
    ffi::epoch_time_ms_t ts;
    ffi::epoch_time_ms_t preamble_ts = get_current_ts();
    std::vector<int8_t> ir_buf;
    std::string logtype;
    std::vector<std::string> reference_messages;
    std::vector<ffi::epoch_time_ms_t> reference_timestamps;
    const std::string timestamp_pattern = "%Y-%m-%d %H:%M:%S,%3";
    const std::string timestamp_pattern_syntax = "yyyy-MM-dd HH:mm:ss";
    const std::string time_zone_id = "Asia/Tokyo";

    REQUIRE(encode_preamble<TestType>(timestamp_pattern, timestamp_pattern_syntax, time_zone_id,
                                      preamble_ts, ir_buf));

    // First message:
    message = "Static <\text>, dictVar1, 123, 456.7, dictVar2, 987, 654.3, end of static text";
    ts = get_next_timestamp_for_test<TestType>();
    REQUIRE(encode_message<TestType>(ts, message, logtype, ir_buf));
    reference_messages.push_back(message);
    reference_timestamps.push_back(ts);

    // Second message:
    message = "Static <\text>, dictVar3, 355.2352512, "
              "23953324532112, python3.4.6, end of static text";
    ts = get_next_timestamp_for_test<TestType>();
    REQUIRE(encode_message<TestType>(ts, message, logtype, ir_buf));
    reference_messages.push_back(message);
    reference_timestamps.push_back(ts);

    const size_t encoded_message_end_pos = ir_buf.size();
    const size_t encoded_message_start_pos = 0;

    ffi::ir_stream::IRBuffer complete_encoding_buffer(ir_buf.data(), ir_buf.size());

    bool is_four_bytes_encoding;
    ffi::ir_stream::TimestampInfo ts_info;
    REQUIRE(ffi::ir_stream::get_encoding_type(complete_encoding_buffer, is_four_bytes_encoding) ==
            IRErrorCode::IRErrorCode_Success);
    REQUIRE(match_encoding_type<TestType>(is_four_bytes_encoding));

    // Test if preamble can be properly decoded
    REQUIRE(decode_preamble<TestType>(complete_encoding_buffer, ts_info, preamble_ts) ==
            IRErrorCode::IRErrorCode_Success);
    REQUIRE(timestamp_pattern_syntax == ts_info.timestamp_pattern_syntax);
    REQUIRE(time_zone_id == ts_info.time_zone_id);
    REQUIRE(timestamp_pattern == ts_info.timestamp_pattern);

    string decoded_message;
    ffi::epoch_time_ms_t timestamp;
    for (size_t ix = 0; ix < reference_messages.size(); ix++) {
        REQUIRE(ffi::ir_stream::IRErrorCode_Success ==
                decode_next_message<TestType>(complete_encoding_buffer, decoded_message,
                                              timestamp));
        REQUIRE(decoded_message == reference_messages[ix]);
        REQUIRE(timestamp == reference_timestamps[ix]);
    }
    REQUIRE(complete_encoding_buffer.get_cursor_pos() == encoded_message_end_pos);
}
