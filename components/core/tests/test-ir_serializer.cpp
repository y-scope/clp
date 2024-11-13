#include <chrono>
#include <cstdint>
#include <filesystem>
#include <numeric>
#include <string>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/ffi/ir_stream/decoding_methods.hpp"
#include "../src/clp/ir/constants.hpp"
#include "../src/clp/ir/LogEventDeserializer.hpp"
#include "../src/clp/ir/LogEventSerializer.hpp"
#include "../src/clp/ir/types.hpp"
#include "../src/clp/streaming_compression/zstd/Decompressor.hpp"

using clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
using clp::ir::cIrFileExtension;
using clp::ir::eight_byte_encoded_variable_t;
using clp::ir::epoch_time_ms_t;
using clp::ir::four_byte_encoded_variable_t;
using clp::ir::LogEventDeserializer;
using clp::ir::LogEventSerializer;
using clp::streaming_compression::zstd::Decompressor;
using std::chrono::milliseconds;
using std::chrono::system_clock;
using std::is_same_v;
using std::string;
using std::vector;

namespace {
struct TestLogEvent {
    epoch_time_ms_t timestamp;
    string msg;
};
}  // namespace

TEMPLATE_TEST_CASE(
        "Encode and serialize log events",
        "[ir][serialize-log-event]",
        four_byte_encoded_variable_t,
        eight_byte_encoded_variable_t
) {
    vector<TestLogEvent> test_log_events;

    auto const ts_1 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    vector<string> const log_event_1_tokens
            = {"Here is the first string with a small int ",
               "4938",
               " and a medium int ",
               std::to_string(INT32_MAX),
               " and a very large int ",
               std::to_string(INT64_MAX),
               " and a small float ",
               "0.1",
               "\n"};
    auto const log_event_1
            = std::accumulate(log_event_1_tokens.begin(), log_event_1_tokens.end(), string(""));
    test_log_events.push_back({ts_1, log_event_1});

    auto const ts_2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    vector<string> const log_event_2_tokens
            = {"Here is the second string with a medium float ",
               "-25.519686",
               " and a high precision float ",
               "-25.5196868642755",
               " and a weird float ",
               "-00.00",
               " and a string with numbers ",
               "bin/python2.7.3",
               " and another string with numbers ",
               "abc123",
               "\n"};
    auto const log_event_2
            = std::accumulate(log_event_2_tokens.begin(), log_event_2_tokens.end(), string(""));
    test_log_events.push_back({ts_2, log_event_2});

    string ir_test_file = "ir_serializer_test";
    ir_test_file += cIrFileExtension;

    LogEventSerializer<TestType> serializer;
    REQUIRE(serializer.open(ir_test_file));
    // Test serializing log events
    for (auto const& test_log_event : test_log_events) {
        REQUIRE(serializer.serialize_log_event(test_log_event.timestamp, test_log_event.msg));
    }
    serializer.close();

    Decompressor ir_reader;
    ir_reader.open(ir_test_file);

    bool uses_four_byte_encoding{false};
    REQUIRE(
            (IRErrorCode_Success
             == clp::ffi::ir_stream::get_encoding_type(ir_reader, uses_four_byte_encoding))
    );
    REQUIRE((is_same_v<TestType, four_byte_encoded_variable_t> == uses_four_byte_encoding));

    auto result = LogEventDeserializer<TestType>::create(ir_reader);
    REQUIRE((false == result.has_error()));
    auto& deserializer = result.value();

    // Decode and deserialize all expected log events
    for (auto const& test_log_event : test_log_events) {
        auto deserialized_result = deserializer.deserialize_log_event();
        REQUIRE((false == deserialized_result.has_error()));

        auto& log_event = deserialized_result.value();
        auto const decoded_message = log_event.get_message().decode_and_unparse();
        REQUIRE(decoded_message.has_value());

        REQUIRE((decoded_message.value() == test_log_event.msg));
        REQUIRE((log_event.get_timestamp() == test_log_event.timestamp));
    }
    // Try decoding a nonexistent log event
    auto deserialized_result = deserializer.deserialize_log_event();
    REQUIRE(deserialized_result.has_error());

    std::filesystem::remove(ir_test_file);
}
