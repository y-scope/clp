#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/ErrorCode.hpp"
#include "../src/clp/ffi/ir_stream/decoding_methods.hpp"
#include "../src/clp/FileReader.hpp"
#include "../src/clp/ir/constants.hpp"
#include "../src/clp/ir/LogEventDeserializer.hpp"
#include "../src/clp/ir/LogEventSerializer.hpp"
#include "../src/clp/streaming_compression/zstd/Decompressor.hpp"
#include "../src/clp/Utils.hpp"

using clp::ir::eight_byte_encoded_variable_t;
using clp::ir::four_byte_encoded_variable_t;
using std::string;
using std::vector;

using clp::ffi::ir_stream::deserialize_log_event;

using std::chrono::milliseconds;
using std::chrono::system_clock;
using std::is_same_v;

using clp::ErrorCode_Success;
using clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success;
using clp::FileReader;
using clp::ir::cIrFileExtension;
using clp::ir::epoch_time_ms_t;
using clp::ir::LogEventDeserializer;
using clp::ir::LogEventSerializer;
using clp::streaming_compression::zstd::Decompressor;

namespace {
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
}  // namespace

/*
 * The test case only covers four byte encoding because decompressor
 * does not support eight bytes encoding yet.
 */
TEMPLATE_TEST_CASE(
        "Serialize log events",
        "[ir][serialize-log-event]",
        four_byte_encoded_variable_t
) {
    string message;
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
    size_t var_ix = 0;

    string first_log_event = "here is first string with a small int " + var_strs[var_ix++];
    epoch_time_ms_t first_ts
            = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    first_log_event += " and a medium int " + var_strs[var_ix++];
    first_log_event += " and a very large int " + var_strs[var_ix++];
    first_log_event += " and a small float " + var_strs[var_ix++];
    first_log_event += "\n";

    string second_log_event = "here is second string with a medium float " + var_strs[var_ix++];
    epoch_time_ms_t second_ts
            = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    second_log_event += " and a weird float " + var_strs[var_ix++];
    second_log_event += " and a string with numbers " + var_strs[var_ix++];
    second_log_event += " and another string with numbers " + var_strs[var_ix++];
    second_log_event += "\n";

    // Create directory for serialized ir output
    string const ir_serializer_dir_path = "unit-test-ir_serializer/";
    REQUIRE(ErrorCode_Success == clp::create_directory_structure(ir_serializer_dir_path, 0700));

    auto ir_test_file = ir_serializer_dir_path + "test";
    ir_test_file += cIrFileExtension;

    REQUIRE(serializer.open(ir_test_file));
    // Test serializing log events
    REQUIRE(serializer.serialize_log_event(first_ts, first_log_event));
    REQUIRE(serializer.serialize_log_event(second_ts, second_log_event));
    serializer.close();

    Decompressor ir_reader;
    ir_reader.open(ir_test_file);

    bool uses_four_byte_encoding{false};
    REQUIRE(IRErrorCode_Success
            == clp::ffi::ir_stream::get_encoding_type(ir_reader, uses_four_byte_encoding));
    REQUIRE(true == uses_four_byte_encoding);

    auto result = LogEventDeserializer<TestType>::create(ir_reader);
    REQUIRE(false == result.has_error());
    auto deserializer_inst = std::move(result.value());

    string decoded_message{};
    // Deserialize the first log event from the IR
    auto deserialized_result = deserializer_inst.deserialize_log_event();
    REQUIRE(false == deserialized_result.has_error());

    // Deserialize the log event
    auto log_event = deserialized_result.value();
    REQUIRE(clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success
            == deserialize_log_event(log_event, decoded_message));

    // Compare decoded message and timestamp
    REQUIRE(decoded_message == first_log_event);
    REQUIRE(log_event.get_timestamp() == first_ts);

    // Deserialize the second log event from the IR
    deserialized_result = deserializer_inst.deserialize_log_event();
    REQUIRE(false == deserialized_result.has_error());

    // Deserialize the log event
    log_event = deserialized_result.value();
    decoded_message.clear();
    REQUIRE(clp::ffi::ir_stream::IRErrorCode::IRErrorCode_Success
            == deserialize_log_event(log_event, decoded_message));

    // Compare decoded message and timestamp
    REQUIRE(decoded_message == second_log_event);
    REQUIRE(log_event.get_timestamp() == second_ts);

    // Try decoding non-existing message
    deserialized_result = deserializer_inst.deserialize_log_event();
    REQUIRE(true == deserialized_result.has_error());
}
