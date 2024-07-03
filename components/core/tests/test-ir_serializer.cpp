#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/ErrorCode.hpp"
#include "../src/clp/ffi/ir_stream/decoding_methods.hpp"
#include "../src/clp/ir/constants.hpp"
#include "../src/clp/ir/LogEventDeserializer.hpp"
#include "../src/clp/ir/LogEventSerializer.hpp"
#include "../src/clp/ir/types.hpp"
#include "../src/clp/streaming_compression/zstd/Decompressor.hpp"

using clp::ir::eight_byte_encoded_variable_t;
using clp::ir::four_byte_encoded_variable_t;
using std::string;
using std::vector;

using clp::ffi::ir_stream::deserialize_log_event;

using std::chrono::milliseconds;
using std::chrono::system_clock;
using std::is_same_v;

using clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
using clp::ir::cIrFileExtension;
using clp::ir::epoch_time_ms_t;
using clp::ir::LogEventDeserializer;
using clp::ir::LogEventSerializer;
using clp::streaming_compression::zstd::Decompressor;

namespace {
template <typename encoded_variable_t>
auto match_encoding_type(bool is_four_bytes_encoding) -> bool {
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

struct TestLogEvent {
    epoch_time_ms_t timestamp;
    string msg;
};
}  // namespace

/*
 * The test case only covers four byte encoding because decompressor
 * does not support eight bytes encoding yet.
 */
TEMPLATE_TEST_CASE(
        "Serialize log events",
        "[ir][serialize-log-event]",
        four_byte_encoded_variable_t,
        eight_byte_encoded_variable_t
) {
    LogEventSerializer<TestType> serializer;

    // Test encoding with serializer
    vector<string> var_strs
            = {"4938",
               std::to_string(INT32_MAX),
               std::to_string(INT64_MAX),
               "0.1",
               "-25.519686",
               "-25.5196868642755",
               "-00.00",
               "bin/python2.7.3",
               "abc123"};
    size_t var_ix{0};

    vector<TestLogEvent> test_log_events;

    auto const first_ts
            = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    string first_log_event = "here is first string with a small int " + var_strs[var_ix++];
    first_log_event += " and a medium int " + var_strs[var_ix++];
    first_log_event += " and a very large int " + var_strs[var_ix++];
    first_log_event += " and a small float " + var_strs[var_ix++];
    first_log_event += "\n";
    test_log_events.push_back({first_ts, first_log_event});

    auto const second_ts
            = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    auto second_log_event = "here is second string with a medium float " + var_strs[var_ix++];
    second_log_event += " and a weird float " + var_strs[var_ix++];
    second_log_event += " and a string with numbers " + var_strs[var_ix++];
    second_log_event += " and another string with numbers " + var_strs[var_ix++];
    second_log_event += "\n";
    test_log_events.push_back({second_ts, second_log_event});

    string ir_test_file = "ir_serializer_test";
    ir_test_file += cIrFileExtension;

    REQUIRE(serializer.open(ir_test_file));
    // Test serializing log events
    for (auto const& test_log_event: test_log_events) {
        REQUIRE(serializer.serialize_log_event(test_log_event.timestamp, test_log_event.msg));
    }
    serializer.close();

    Decompressor ir_reader;
    ir_reader.open(ir_test_file);

    bool uses_four_byte_encoding{false};
    REQUIRE(IRErrorCode_Success
            == clp::ffi::ir_stream::get_encoding_type(ir_reader, uses_four_byte_encoding));
    REQUIRE(match_encoding_type<TestType>(uses_four_byte_encoding));

    auto result = LogEventDeserializer<TestType>::create(ir_reader);
    REQUIRE(false == result.has_error());
    auto& deserializer_inst = result.value();

    for (auto const& test_log_event: test_log_events) {
        string decoded_message{};
        // Deserialize the first log event from the IR
        auto deserialized_result = deserializer_inst.deserialize_log_event();
        REQUIRE(false == deserialized_result.has_error());

        // Deserialize the log event
        auto log_event = deserialized_result.value();
        REQUIRE(clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success
                == deserialize_log_event(log_event, decoded_message));

        // Compare decoded message and timestamp
        REQUIRE(decoded_message == test_log_event.msg);
        REQUIRE(log_event.get_timestamp() == test_log_event.timestamp);
    }
    // Try decoding non-existing message
    auto deserialized_result = deserializer_inst.deserialize_log_event();
    REQUIRE(deserialized_result.has_error());

    std::filesystem::remove(ir_test_file);
}
