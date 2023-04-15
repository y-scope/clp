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
using ffi::ir_stream::IR_ErrorCode;
using std::chrono::milliseconds;
using std::chrono::system_clock;

TEST_CASE("check_encoding_type", "[ffi][check_encoding_type]") {

    std::vector<int8_t> empty_ir_buf;
    size_t cursor = 0;
    bool is_four_bytes_encoding;
    REQUIRE(ffi::ir_stream::get_encoding_type(empty_ir_buf, cursor, is_four_bytes_encoding) == IR_ErrorCode::ErrorCode_InComplete_IR);

    std::vector<int8_t> invalid_ir_buf{0x02,0x43,0x24,0x34};
    cursor = 0;
    REQUIRE(ffi::ir_stream::get_encoding_type(invalid_ir_buf, cursor, is_four_bytes_encoding) == IR_ErrorCode::ErrorCode_Corrupted_IR);

    std::vector<int8_t> ir_buf;
    ir_buf.resize(ffi::ir_stream::cProtocol::MagicNumberLength);
    memcpy(ir_buf.data(), ffi::ir_stream::cProtocol::EightByteEncodingMagicNumber, ffi::ir_stream::cProtocol::MagicNumberLength);
    cursor = 0;
    REQUIRE(ffi::ir_stream::get_encoding_type(ir_buf, cursor, is_four_bytes_encoding) == IR_ErrorCode::ErrorCode_Success);
    REQUIRE(false == is_four_bytes_encoding);

    memcpy(ir_buf.data(), ffi::ir_stream::cProtocol::FourByteEncodingMagicNumber, ffi::ir_stream::cProtocol::MagicNumberLength);
    cursor = 0;
    REQUIRE(ffi::ir_stream::get_encoding_type(ir_buf, cursor, is_four_bytes_encoding) == IR_ErrorCode::ErrorCode_Success);
    REQUIRE(is_four_bytes_encoding);

    // add some random values
    ir_buf.push_back(0x04);
    cursor = 0;
    REQUIRE(ffi::ir_stream::get_encoding_type(ir_buf, cursor, is_four_bytes_encoding) == IR_ErrorCode::ErrorCode_Success);
    REQUIRE(is_four_bytes_encoding);
}

TEST_CASE("decode_preamble", "[ffi][check_encoding_type]") {

    string message = "Static text, dictVar1, 123, 456.7, dictVar2, 987, 654.3";

    std::vector<int8_t> ir_buf;
    const std::string timestamp_pattern = "%Y-%m-%d %H:%M:%S,%3";
    const std::string timestamp_pattern_syntax = "yyyy-MM-dd HH:mm:ss";
    const std::string time_zone_id = "Asia/Tokyo";
    REQUIRE(ffi::ir_stream::eight_byte_encoding::encode_preamble(timestamp_pattern, timestamp_pattern_syntax, time_zone_id, ir_buf));
    const size_t encoded_preamble_end_pos = ir_buf.size();
    ir_buf.push_back(0x34);

    ffi::ir_stream::eight_byte_encoding::TimestampInfo ts_info;
    size_t cursor_pos = 0;
    bool is_four_bytes_encoding;
    REQUIRE(ffi::ir_stream::get_encoding_type(ir_buf, cursor_pos, is_four_bytes_encoding) == IR_ErrorCode::ErrorCode_Success);
    REQUIRE(false == is_four_bytes_encoding);
    REQUIRE(ffi::ir_stream::cProtocol::MagicNumberLength == cursor_pos);

    // Test if preamble can be properly decoded
    REQUIRE(ffi::ir_stream::eight_byte_encoding::decode_preamble(ir_buf, ts_info, cursor_pos) == IR_ErrorCode::ErrorCode_Success);

    REQUIRE(timestamp_pattern_syntax == ts_info.timestamp_pattern_syntax);
    REQUIRE(time_zone_id == ts_info.time_zone_id);
    REQUIRE(timestamp_pattern == ts_info.timestamp_pattern);
    REQUIRE(encoded_preamble_end_pos == cursor_pos);

    // Test if incomplete IR can be detected.
    ir_buf.resize(encoded_preamble_end_pos - 1);
    cursor_pos = ffi::ir_stream::cProtocol::MagicNumberLength;
    REQUIRE(ffi::ir_stream::eight_byte_encoding::decode_preamble(ir_buf, ts_info, cursor_pos) == IR_ErrorCode::ErrorCode_InComplete_IR);

    // Test if corrupted IR can be detected.
    ir_buf.at(ffi::ir_stream::cProtocol::MagicNumberLength) = 0x23;
    cursor_pos = ffi::ir_stream::cProtocol::MagicNumberLength;
    REQUIRE(ffi::ir_stream::eight_byte_encoding::decode_preamble(ir_buf, ts_info, cursor_pos) == IR_ErrorCode::ErrorCode_Corrupted_IR);
}

TEST_CASE("decode_next_message", "[ffi][decode_next_message]") {

    string message = "Static <\text>, dictVar1, 123, 456.7, dictVar2, 987, 654.3, end of static text";

    std::vector<int8_t> ir_buf;
    std::string logtype;
    ffi::ir_stream::eight_byte_encoding::TimestampInfo ts_info;
    ffi::epoch_time_ms_t reference_timestamp = 1234567;
    REQUIRE(true == ffi::ir_stream::eight_byte_encoding::encode_message(reference_timestamp, message, logtype, ir_buf));
    const size_t encoded_message_end_pos = ir_buf.size();
    const size_t encoded_message_start_pos = 0;

    size_t cursor_pos = encoded_message_start_pos;
    string decoded_message;
    ffi::epoch_time_ms_t timestamp;
    REQUIRE(ffi::ir_stream::ErrorCode_Success == ffi::ir_stream::eight_byte_encoding::decode_next_message(ts_info, ir_buf, decoded_message, timestamp, cursor_pos));
    REQUIRE(message == decoded_message);
    REQUIRE(timestamp == reference_timestamp);
    REQUIRE(cursor_pos == encoded_message_end_pos);

    cursor_pos = encoded_message_start_pos + 1;
    REQUIRE(ffi::ir_stream::ErrorCode_Corrupted_IR == ffi::ir_stream::eight_byte_encoding::decode_next_message(ts_info, ir_buf, message, timestamp, cursor_pos));

    cursor_pos = encoded_message_start_pos;
    ir_buf.resize(encoded_message_end_pos - 4);
    REQUIRE(ffi::ir_stream::ErrorCode_InComplete_IR == ffi::ir_stream::eight_byte_encoding::decode_next_message(ts_info, ir_buf, message, timestamp, cursor_pos));
}

TEST_CASE("complete_test", "[ffi][decode_next_message]") {
    string message;
    uint64_t ts;
    std::vector<int8_t> ir_buf;
    std::string logtype;
    std::vector<std::string> reference_messages;
    std::vector<ffi::epoch_time_ms_t> reference_timestamps;
    const std::string timestamp_pattern = "%Y-%m-%d %H:%M:%S,%3";
    const std::string timestamp_pattern_syntax = "yyyy-MM-dd HH:mm:ss";
    const std::string time_zone_id = "Asia/Tokyo";
    REQUIRE(ffi::ir_stream::eight_byte_encoding::encode_preamble(timestamp_pattern, timestamp_pattern_syntax, time_zone_id, ir_buf));


    // First message:
    message = "Static <\text>, dictVar1, 123, 456.7, dictVar2, 987, 654.3, end of static text";
    ts = std::chrono::duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    ffi::ir_stream::eight_byte_encoding::encode_message(ts, message, logtype, ir_buf);
    reference_messages.push_back(message);
    reference_timestamps.push_back(ts);

    // Second message:
    message = "Static <\text>, dictVar3, 355.2352512, 23953324532112, python3.4.6, end of static text";
    ts = std::chrono::duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    ffi::ir_stream::eight_byte_encoding::encode_message(ts, message, logtype, ir_buf);
    reference_messages.push_back(message);
    reference_timestamps.push_back(ts);

    const size_t encoded_message_end_pos = ir_buf.size();
    const size_t encoded_message_start_pos = 0;
    size_t cursor_pos = encoded_message_start_pos;


    bool is_four_bytes_encoding;
    ffi::ir_stream::eight_byte_encoding::TimestampInfo ts_info;
    REQUIRE(ffi::ir_stream::get_encoding_type(ir_buf, cursor_pos, is_four_bytes_encoding) == IR_ErrorCode::ErrorCode_Success);
    REQUIRE(false == is_four_bytes_encoding);

    // Test if preamble can be properly decoded
    REQUIRE(ffi::ir_stream::eight_byte_encoding::decode_preamble(ir_buf, ts_info, cursor_pos) == IR_ErrorCode::ErrorCode_Success);
    REQUIRE(timestamp_pattern_syntax == ts_info.timestamp_pattern_syntax);
    REQUIRE(time_zone_id == ts_info.time_zone_id);
    REQUIRE(timestamp_pattern == ts_info.timestamp_pattern);

    string decoded_message;
    ffi::epoch_time_ms_t timestamp;
    for (size_t ix = 0; ix < reference_messages.size(); ix++) {
        REQUIRE(ffi::ir_stream::ErrorCode_Success == ffi::ir_stream::eight_byte_encoding::decode_next_message(ts_info, ir_buf, decoded_message, timestamp, cursor_pos));
        REQUIRE(decoded_message == reference_messages[ix]);
        REQUIRE(timestamp == reference_timestamps[ix]);
    }
    REQUIRE(cursor_pos == encoded_message_end_pos);
}