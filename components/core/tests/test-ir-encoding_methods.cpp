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

    const std::vector<int8_t> empty_ir_vec;
    ffi::ir_stream::IRBuffer empty_ir_buffer(empty_ir_vec.data(), empty_ir_vec.size());

    bool is_four_bytes_encoding;
    REQUIRE(ffi::ir_stream::get_encoding_type(empty_ir_buffer, is_four_bytes_encoding) == IR_ErrorCode::ErrorCode_InComplete_IR);

    const std::vector<int8_t> invalid_ir_vec{0x02,0x43,0x24,0x34};
    ffi::ir_stream::IRBuffer invalid_ir_buffer(invalid_ir_vec.data(), invalid_ir_vec.size());

    REQUIRE(ffi::ir_stream::get_encoding_type(invalid_ir_buffer, is_four_bytes_encoding) == IR_ErrorCode::ErrorCode_Corrupted_IR);

    // Test eight bytes encoding
    std::vector<int8_t> eight_byte_encoding_vec;
    eight_byte_encoding_vec.resize(ffi::ir_stream::cProtocol::MagicNumberLength);
    memcpy(eight_byte_encoding_vec.data(), ffi::ir_stream::cProtocol::EightByteEncodingMagicNumber, ffi::ir_stream::cProtocol::MagicNumberLength);

    ffi::ir_stream::IRBuffer eight_byte_ir_buffer(eight_byte_encoding_vec.data(), eight_byte_encoding_vec.size());
    REQUIRE(ffi::ir_stream::get_encoding_type(eight_byte_ir_buffer, is_four_bytes_encoding) == IR_ErrorCode::ErrorCode_Success);
    REQUIRE(false == is_four_bytes_encoding);


    std::vector<int8_t> four_byte_encoding_vec;
    four_byte_encoding_vec.resize(ffi::ir_stream::cProtocol::MagicNumberLength);
    memcpy(four_byte_encoding_vec.data(), ffi::ir_stream::cProtocol::FourByteEncodingMagicNumber, ffi::ir_stream::cProtocol::MagicNumberLength);

    ffi::ir_stream::IRBuffer four_byte_ir_buffer(four_byte_encoding_vec.data(), four_byte_encoding_vec.size());
    REQUIRE(ffi::ir_stream::get_encoding_type(four_byte_ir_buffer, is_four_bytes_encoding) == IR_ErrorCode::ErrorCode_Success);
    REQUIRE(is_four_bytes_encoding);
}

TEST_CASE("decode_preamble-8bytes", "[ffi][check_encoding_type]") {

    string message = "Static text, dictVar1, 123, 456.7, dictVar2, 987, 654.3";

    std::vector<int8_t> ir_buf;
    const std::string timestamp_pattern = "%Y-%m-%d %H:%M:%S,%3";
    const std::string timestamp_pattern_syntax = "yyyy-MM-dd HH:mm:ss";
    const std::string time_zone_id = "Asia/Tokyo";
    REQUIRE(ffi::ir_stream::eight_byte_encoding::encode_preamble(timestamp_pattern, timestamp_pattern_syntax, time_zone_id, ir_buf));
    const size_t encoded_preamble_end_pos = ir_buf.size();
    ir_buf.push_back(0x34);

    ffi::ir_stream::TimestampInfo ts_info;
    ffi::ir_stream::IRBuffer eight_bytes_preamble_buffer(ir_buf.data(), ir_buf.size());
    bool is_four_bytes_encoding;
    REQUIRE(ffi::ir_stream::get_encoding_type(eight_bytes_preamble_buffer, is_four_bytes_encoding) == IR_ErrorCode::ErrorCode_Success);
    REQUIRE(false == is_four_bytes_encoding);
    REQUIRE(ffi::ir_stream::cProtocol::MagicNumberLength == eight_bytes_preamble_buffer.cursor_pos);

    // Test if preamble can be properly decoded
    REQUIRE(ffi::ir_stream::eight_byte_encoding::decode_preamble(eight_bytes_preamble_buffer, ts_info) == IR_ErrorCode::ErrorCode_Success);

    REQUIRE(timestamp_pattern_syntax == ts_info.timestamp_pattern_syntax);
    REQUIRE(time_zone_id == ts_info.time_zone_id);
    REQUIRE(timestamp_pattern == ts_info.timestamp_pattern);
    REQUIRE(encoded_preamble_end_pos == eight_bytes_preamble_buffer.cursor_pos);

    // Test if incomplete IR can be detected.
    ir_buf.resize(encoded_preamble_end_pos - 1);
    ffi::ir_stream::IRBuffer incomplete_preamble_buffer (ir_buf.data(), ir_buf.size());
    incomplete_preamble_buffer.cursor_pos = ffi::ir_stream::cProtocol::MagicNumberLength;

    REQUIRE(ffi::ir_stream::eight_byte_encoding::decode_preamble(incomplete_preamble_buffer, ts_info) == IR_ErrorCode::ErrorCode_InComplete_IR);

    // Test if corrupted IR can be detected.
    ir_buf.at(ffi::ir_stream::cProtocol::MagicNumberLength) = 0x23;
    ffi::ir_stream::IRBuffer corrupted_preamble_buffer (ir_buf.data(), ir_buf.size());
    incomplete_preamble_buffer.cursor_pos = ffi::ir_stream::cProtocol::MagicNumberLength;

    REQUIRE(ffi::ir_stream::eight_byte_encoding::decode_preamble(corrupted_preamble_buffer, ts_info) == IR_ErrorCode::ErrorCode_Corrupted_IR);
}

//TEST_CASE("decode_preamble-4bytes", "[ffi][check_encoding_type]") {
//
//    string message = "Static text, dictVar1, 123, 456.7, dictVar2, 987, 654.3";
//
//    std::vector<int8_t> ir_buf;
//    const std::string timestamp_pattern = "%Y-%m-%d %H:%M:%S,%3";
//    const std::string timestamp_pattern_syntax = "yyyy-MM-dd HH:mm:ss";
//    const std::string time_zone_id = "Asia/Tokyo";
//    const ffi::epoch_time_ms_t reference_ts = std::chrono::duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
//    REQUIRE(ffi::ir_stream::four_byte_encoding::encode_preamble(timestamp_pattern, timestamp_pattern_syntax, time_zone_id, reference_ts, ir_buf));
//    const size_t encoded_preamble_end_pos = ir_buf.size();
//    ir_buf.push_back(0x34);
//
//    ffi::ir_stream::TimestampInfo ts_info;
//    ffi::epoch_time_ms_t decoded_ts;
//    size_t cursor_pos = 0;
//    bool is_four_bytes_encoding;
//    REQUIRE(ffi::ir_stream::get_encoding_type(ir_buf, cursor_pos, is_four_bytes_encoding) == IR_ErrorCode::ErrorCode_Success);
//    REQUIRE(true == is_four_bytes_encoding);
//    REQUIRE(ffi::ir_stream::cProtocol::MagicNumberLength == cursor_pos);
//
//    // Test if preamble can be properly decoded
//    REQUIRE(ffi::ir_stream::four_byte_encoding::decode_preamble(ir_buf, ts_info, cursor_pos, decoded_ts) == IR_ErrorCode::ErrorCode_Success);
//
//    REQUIRE(timestamp_pattern_syntax == ts_info.timestamp_pattern_syntax);
//    REQUIRE(time_zone_id == ts_info.time_zone_id);
//    REQUIRE(timestamp_pattern == ts_info.timestamp_pattern);
//    REQUIRE(reference_ts == decoded_ts);
//    REQUIRE(encoded_preamble_end_pos == cursor_pos);
//
//    // Test if incomplete IR can be detected.
//    ir_buf.resize(encoded_preamble_end_pos - 1);
//    cursor_pos = ffi::ir_stream::cProtocol::MagicNumberLength;
//    REQUIRE(ffi::ir_stream::four_byte_encoding::decode_preamble(ir_buf, ts_info, cursor_pos, decoded_ts) == IR_ErrorCode::ErrorCode_InComplete_IR);
//
//    // Test if corrupted IR can be detected.
//    ir_buf.at(ffi::ir_stream::cProtocol::MagicNumberLength) = 0x23;
//    cursor_pos = ffi::ir_stream::cProtocol::MagicNumberLength;
//    REQUIRE(ffi::ir_stream::four_byte_encoding::decode_preamble(ir_buf, ts_info, cursor_pos, decoded_ts) == IR_ErrorCode::ErrorCode_Corrupted_IR);
//}

TEST_CASE("decode_next_message-8bytes", "[ffi][decode_next_message]") {

    string message = "Static <\text>, dictVar1, 123, 456.7, dictVar2, 987, 654.3, end of static text";

    std::vector<int8_t> ir_buf;
    std::string logtype;
    ffi::epoch_time_ms_t reference_timestamp = 1234567;
    REQUIRE(true == ffi::ir_stream::eight_byte_encoding::encode_message(reference_timestamp, message, logtype, ir_buf));
    const size_t encoded_message_end_pos = ir_buf.size();
    const size_t encoded_message_start_pos = 0;

    string decoded_message;
    ffi::epoch_time_ms_t timestamp;
    ffi::ir_stream::IRBuffer encoded_message_buffer(ir_buf.data(), ir_buf.size());

    REQUIRE(ffi::ir_stream::ErrorCode_Success == ffi::ir_stream::eight_byte_encoding::decode_next_message(encoded_message_buffer, decoded_message, timestamp));
    REQUIRE(message == decoded_message);
    REQUIRE(timestamp == reference_timestamp);
    REQUIRE(encoded_message_buffer.cursor_pos == encoded_message_end_pos);

    encoded_message_buffer.cursor_pos = encoded_message_start_pos + 1;
    REQUIRE(ffi::ir_stream::ErrorCode_Corrupted_IR == ffi::ir_stream::eight_byte_encoding::decode_next_message(encoded_message_buffer, message, timestamp));

    ir_buf.resize(encoded_message_end_pos - 4);
    ffi::ir_stream::IRBuffer incomplete_message_buffer(ir_buf.data(), ir_buf.size());
    REQUIRE(ffi::ir_stream::ErrorCode_InComplete_IR == ffi::ir_stream::eight_byte_encoding::decode_next_message(incomplete_message_buffer, message, timestamp));
}

//TEST_CASE("decode_next_message-4bytes", "[ffi][decode_next_message]") {
//
//    string message = "Static <\text>, dictVar1, 123, 456345232.7234223, dictVar2, 987, 654.3, end of static text";
//
//    std::vector<int8_t> ir_buf;
//    std::string logtype;
//    // ensure that decoder can handle negative ts_delta
//    ffi::epoch_time_ms_t reference_delta_ts_negative = -5;
//    ffi::epoch_time_ms_t reference_delta_ts = 23453;
//    REQUIRE(true == ffi::ir_stream::four_byte_encoding::encode_message(reference_delta_ts_negative, message, logtype, ir_buf));
//    REQUIRE(true == ffi::ir_stream::four_byte_encoding::encode_message(reference_delta_ts, message, logtype, ir_buf));
//    const size_t encoded_message_end_pos = ir_buf.size();
//    const size_t encoded_message_start_pos = 0;
//
//    size_t cursor_pos = encoded_message_start_pos;
//    string decoded_message;
//    ffi::epoch_time_ms_t delta_ts;
//    REQUIRE(ffi::ir_stream::ErrorCode_Success == ffi::ir_stream::four_byte_encoding::decode_next_message(ir_buf, decoded_message, delta_ts, cursor_pos));
//    REQUIRE(message == decoded_message);
//    REQUIRE(delta_ts == reference_delta_ts_negative);
//
//    size_t first_msg_end_pos = cursor_pos;
//    REQUIRE(ffi::ir_stream::ErrorCode_Success == ffi::ir_stream::four_byte_encoding::decode_next_message(ir_buf, decoded_message, delta_ts, cursor_pos));
//    REQUIRE(delta_ts == reference_delta_ts);
//    REQUIRE(cursor_pos == encoded_message_end_pos);
//
//
//    cursor_pos = encoded_message_start_pos + 1;
//    REQUIRE(ffi::ir_stream::ErrorCode_Corrupted_IR == ffi::ir_stream::four_byte_encoding::decode_next_message(ir_buf, message, delta_ts, cursor_pos));
//
//    cursor_pos = first_msg_end_pos;
//    ir_buf.resize(encoded_message_end_pos - 4);
//    REQUIRE(ffi::ir_stream::ErrorCode_InComplete_IR == ffi::ir_stream::four_byte_encoding::decode_next_message(ir_buf, message, delta_ts, cursor_pos));
//}
//
//TEST_CASE("complete_test-8bytes", "[ffi][decode_next_message]") {
//    string message;
//    ffi::epoch_time_ms_t ts;
//    std::vector<int8_t> ir_buf;
//    std::string logtype;
//    std::vector<std::string> reference_messages;
//    std::vector<ffi::epoch_time_ms_t> reference_timestamps;
//    const std::string timestamp_pattern = "%Y-%m-%d %H:%M:%S,%3";
//    const std::string timestamp_pattern_syntax = "yyyy-MM-dd HH:mm:ss";
//    const std::string time_zone_id = "Asia/Tokyo";
//    REQUIRE(ffi::ir_stream::eight_byte_encoding::encode_preamble(timestamp_pattern, timestamp_pattern_syntax, time_zone_id, ir_buf));
//
//
//    // First message:
//    message = "Static <\text>, dictVar1, 123, 456.7, dictVar2, 987, 654.3, end of static text";
//    ts = std::chrono::duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
//    ffi::ir_stream::eight_byte_encoding::encode_message(ts, message, logtype, ir_buf);
//    reference_messages.push_back(message);
//    reference_timestamps.push_back(ts);
//
//    // Second message:
//    message = "Static <\text>, dictVar3, 355.2352512, 23953324532112, python3.4.6, end of static text";
//    ts = std::chrono::duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
//    ffi::ir_stream::eight_byte_encoding::encode_message(ts, message, logtype, ir_buf);
//    reference_messages.push_back(message);
//    reference_timestamps.push_back(ts);
//
//    const size_t encoded_message_end_pos = ir_buf.size();
//    const size_t encoded_message_start_pos = 0;
//    size_t cursor_pos = encoded_message_start_pos;
//
//
//    bool is_four_bytes_encoding;
//    ffi::ir_stream::TimestampInfo ts_info;
//    REQUIRE(ffi::ir_stream::get_encoding_type(ir_buf, cursor_pos, is_four_bytes_encoding) == IR_ErrorCode::ErrorCode_Success);
//    REQUIRE(false == is_four_bytes_encoding);
//
//    // Test if preamble can be properly decoded
//    REQUIRE(ffi::ir_stream::eight_byte_encoding::decode_preamble(ir_buf, ts_info, cursor_pos) == IR_ErrorCode::ErrorCode_Success);
//    REQUIRE(timestamp_pattern_syntax == ts_info.timestamp_pattern_syntax);
//    REQUIRE(time_zone_id == ts_info.time_zone_id);
//    REQUIRE(timestamp_pattern == ts_info.timestamp_pattern);
//
//    string decoded_message;
//    ffi::epoch_time_ms_t timestamp;
//    for (size_t ix = 0; ix < reference_messages.size(); ix++) {
//        REQUIRE(ffi::ir_stream::ErrorCode_Success == ffi::ir_stream::eight_byte_encoding::decode_next_message(ir_buf, decoded_message, timestamp, cursor_pos));
//        REQUIRE(decoded_message == reference_messages[ix]);
//        REQUIRE(timestamp == reference_timestamps[ix]);
//    }
//    REQUIRE(cursor_pos == encoded_message_end_pos);
//}
//
//TEST_CASE("complete_test-4bytes", "[ffi][decode_next_message]") {
//    string message;
//    ffi::epoch_time_ms_t ts;
//    ffi::epoch_time_ms_t ts_delta;
//    std::vector<int8_t> ir_buf;
//    std::string logtype;
//    std::vector<std::string> reference_messages;
//    std::vector<ffi::epoch_time_ms_t> reference_timestamps;
//    const std::string timestamp_pattern = "%Y-%m-%d %H:%M:%S,%3";
//    const std::string timestamp_pattern_syntax = "yyyy-MM-dd HH:mm:ss";
//    const std::string time_zone_id = "Asia/Tokyo";
//    const ffi::epoch_time_ms_t base_reference_ts = std::chrono::duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
//
//    ts = base_reference_ts;
//    REQUIRE(ffi::ir_stream::four_byte_encoding::encode_preamble(timestamp_pattern, timestamp_pattern_syntax, time_zone_id, base_reference_ts, ir_buf));
//
//
//    // First message:
//    message = "Static <\text>, dictVar1, 123, 456.7, dictVar2, 987, 654.3, end of static text";
//    ts_delta = std::chrono::duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - ts;
//    ts += ts_delta;
//    ffi::ir_stream::four_byte_encoding::encode_message(ts_delta, message, logtype, ir_buf);
//    reference_messages.push_back(message);
//    reference_timestamps.push_back(ts_delta);
//
//    // Second message:
//    message = "Static <\text>, dictVar3, 355.2352512, 23953324532112, python3.4.6, end of static text";
//    ts_delta = std::chrono::duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - ts;
//    ts += ts_delta;
//    ffi::ir_stream::four_byte_encoding::encode_message(ts_delta, message, logtype, ir_buf);
//    reference_messages.push_back(message);
//    reference_timestamps.push_back(ts_delta);
//
//    const size_t encoded_message_end_pos = ir_buf.size();
//    const size_t encoded_message_start_pos = 0;
//    size_t cursor_pos = encoded_message_start_pos;
//
//
//    bool is_four_bytes_encoding;
//    ffi::ir_stream::TimestampInfo ts_info;
//    REQUIRE(ffi::ir_stream::get_encoding_type(ir_buf, cursor_pos, is_four_bytes_encoding) == IR_ErrorCode::ErrorCode_Success);
//    REQUIRE(true == is_four_bytes_encoding);
//
//    // Test if preamble can be properly decoded
//    REQUIRE(ffi::ir_stream::four_byte_encoding::decode_preamble(ir_buf, ts_info, cursor_pos, ts) == IR_ErrorCode::ErrorCode_Success);
//    REQUIRE(timestamp_pattern_syntax == ts_info.timestamp_pattern_syntax);
//    REQUIRE(time_zone_id == ts_info.time_zone_id);
//    REQUIRE(timestamp_pattern == ts_info.timestamp_pattern);
//    REQUIRE(base_reference_ts == ts);
//
//    string decoded_message;
//    ffi::epoch_time_ms_t timestamp;
//    for (size_t ix = 0; ix < reference_messages.size(); ix++) {
//        REQUIRE(ffi::ir_stream::ErrorCode_Success == ffi::ir_stream::four_byte_encoding::decode_next_message(ir_buf, decoded_message, timestamp, cursor_pos));
//        REQUIRE(decoded_message == reference_messages[ix]);
//        REQUIRE(timestamp == reference_timestamps[ix]);
//    }
//    REQUIRE(cursor_pos == encoded_message_end_pos);
//}