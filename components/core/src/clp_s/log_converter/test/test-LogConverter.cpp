#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/BufferReader.hpp>
#include <clp/ffi/ir_stream/decoding_methods.hpp>
#include <clp/ffi/ir_stream/Deserializer.hpp>
#include <clp/ffi/ir_stream/IrUnitType.hpp>
#include <clp/ffi/KeyValuePairLogEvent.hpp>
#include <clp/ffi/SchemaTree.hpp>
#include <clp/FileReader.hpp>
#include <clp/time_types.hpp>
#include <clp_s/InputConfig.hpp>
#include <clp_s/log_converter/LogConverter.hpp>
#include <clp_s/log_converter/LogSerializer.hpp>

#include "TestOutputCleaner.hpp"

namespace clp_s::log_converter::test {
namespace {
constexpr std::string_view cTestOutputDir{"test-log-converter-output"};

/**
 * A log event parsed by `LogConverter`, reconstructed from the serialized KV-IR.
 */
struct ParsedEvent {
    std::optional<std::string> timestamp;
    std::string message;
};

/**
 * Implements `clp::ffi::ir_stream::IrUnitHandlerReq` to collect deserialized log events.
 */
class IrUnitHandler {
public:
    [[nodiscard]] auto handle_log_event(
            clp::ffi::KeyValuePairLogEvent const& log_event,
            [[maybe_unused]] size_t log_event_idx
    ) -> clp::ffi::ir_stream::IRErrorCode {
        auto const json_result{log_event.serialize_to_json()};
        if (json_result.has_error()) {
            return clp::ffi::ir_stream::IRErrorCode_Decode_Error;
        }
        auto const& user_gen{json_result.value().second};
        ParsedEvent event;
        if (user_gen.contains(LogSerializer::cTimestampKey)) {
            event.timestamp = user_gen.at(LogSerializer::cTimestampKey).template get<std::string>();
        }
        if (false == user_gen.contains(LogSerializer::cMessageKey)) {
            return clp::ffi::ir_stream::IRErrorCode_Decode_Error;
        }
        event.message = user_gen.at(LogSerializer::cMessageKey).template get<std::string>();
        m_events.emplace_back(std::move(event));
        return clp::ffi::ir_stream::IRErrorCode_Success;
    }

    [[nodiscard]] static auto handle_utc_offset_change(
            [[maybe_unused]] clp::UtcOffset utc_offset_old,
            [[maybe_unused]] clp::UtcOffset utc_offset_new
    ) -> clp::ffi::ir_stream::IRErrorCode {
        return clp::ffi::ir_stream::IRErrorCode_Success;
    }

    [[nodiscard]] static auto handle_schema_tree_node_insertion(
            [[maybe_unused]] bool is_auto_generated,
            [[maybe_unused]] clp::ffi::SchemaTree::NodeLocator schema_tree_node_locator,
            [[maybe_unused]] std::shared_ptr<clp::ffi::SchemaTree const> const& schema_tree
    ) -> clp::ffi::ir_stream::IRErrorCode {
        return clp::ffi::ir_stream::IRErrorCode_Success;
    }

    [[nodiscard]] static auto handle_end_of_stream() -> clp::ffi::ir_stream::IRErrorCode {
        return clp::ffi::ir_stream::IRErrorCode_Success;
    }

    // Methods
    [[nodiscard]] auto get_events() const -> std::vector<ParsedEvent> const& { return m_events; }

private:
    std::vector<ParsedEvent> m_events;
};

/**
 * Converts a string into KV-IR and reads back the parsed log events.
 * @param input The unstructured log text to convert.
 * @param max_buffer_size The maximum size of the converter's internal buffer.
 * @param initial_buffer_size The initial size of the converter's internal buffer.
 * @return A result containing the parsed log events, or the error code of the failed conversion.
 */
[[nodiscard]] auto
convert_and_read_back(std::string_view input, size_t max_buffer_size, size_t initial_buffer_size)
        -> ystdlib::error_handling::Result<std::vector<ParsedEvent>> {
    std::filesystem::create_directories(cTestOutputDir);
    clp::BufferReader source_reader{input.data(), input.size()};
    clp_s::Path const path{.source = clp_s::InputSource::Filesystem, .path = "test_input"};
    auto converter{LogConverter::create(max_buffer_size, initial_buffer_size)};
    auto const convert_result{converter.convert_file(path, &source_reader, cTestOutputDir, false)};
    if (convert_result.has_error()) {
        return convert_result.error();
    }

    std::filesystem::path converted_path;
    for (auto const& entry : std::filesystem::directory_iterator(cTestOutputDir)) {
        converted_path = entry.path();
    }

    clp::FileReader reader{converted_path.string()};
    auto deserializer_result{
            clp::ffi::ir_stream::Deserializer<IrUnitHandler>::create(reader, IrUnitHandler{})
    };
    if (deserializer_result.has_error()) {
        return deserializer_result.error();
    }
    auto& deserializer{deserializer_result.value()};
    while (true) {
        auto const result{deserializer.deserialize_next_ir_unit(reader)};
        if (result.has_error()) {
            return result.error();
        }
        if (clp::ffi::ir_stream::IrUnitType::EndOfStream == result.value()) {
            break;
        }
    }
    return deserializer.get_ir_unit_handler().get_events();
}

/**
 * Builds a log whose lines all start with a timestamp header.
 * @param num_events The number of log events to generate.
 * @param message_size The number of padding bytes to add to each event's message.
 * @return The generated log.
 */
[[nodiscard]] auto make_numbered_log(size_t num_events, size_t message_size) -> std::string {
    std::string log;
    for (size_t idx{0}; idx < num_events; ++idx) {
        log += "2015-03-23 05:48:30,122 EVT";
        log += std::to_string(idx);
        log += std::string(message_size, '.').append("\n");
    }
    return log;
}
}  // namespace

TEST_CASE("log_converter_parses_single_buffer", "[clp_s][log_converter]") {
    TestOutputCleaner const test_output_cleaner{{std::string{cTestOutputDir}}};

    auto const input{
            std::string{"2015-03-23 05:48:30,122 hello\n"} + "2015-03-23 05:48:31,000 world\n"
    };
    auto const result{convert_and_read_back(input, 64ULL * 1024ULL, 64ULL * 1024ULL)};
    REQUIRE_FALSE(result.has_error());
    auto const& events{result.value()};
    REQUIRE(2ULL == events.size());
    REQUIRE(std::optional<std::string>{"2015-03-23 05:48:30,122"} == events.at(0).timestamp);
    REQUIRE(" hello\n" == events.at(0).message);
    REQUIRE(std::optional<std::string>{"2015-03-23 05:48:31,000"} == events.at(1).timestamp);
    REQUIRE(" world\n" == events.at(1).message);
}

TEST_CASE("log_converter_parses_across_many_refills", "[clp_s][log_converter]") {
    constexpr size_t cNumEvents{200};
    TestOutputCleaner const test_output_cleaner{{std::string{cTestOutputDir}}};

    // A tiny initial buffer forces multiple refills and compactions. This catches the
    // buffer-relative offset corruption that previously crashed the converter.
    auto const input{make_numbered_log(cNumEvents, 8)};
    auto const result{convert_and_read_back(input, 64ULL * 1024ULL, 128ULL)};
    REQUIRE_FALSE(result.has_error());
    auto const& events{result.value()};
    REQUIRE(cNumEvents == events.size());

    std::string reconstructed;
    for (auto const& event : events) {
        REQUIRE(event.timestamp.has_value());
        // The REQUIRE macro isn't recognized as a check.
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        reconstructed += event.timestamp.value();
        reconstructed += event.message;
    }
    REQUIRE(input == reconstructed);
}

TEST_CASE("log_converter_parses_event_larger_than_buffer", "[clp_s][log_converter]") {
    constexpr int cBigStrSize{1024};
    TestOutputCleaner const test_output_cleaner{{std::string{cTestOutputDir}}};

    // An event larger than the initial buffer forces the buffer to grow.
    auto const input{
            std::string{"2015-03-23 05:48:30,122 <"} + std::string(cBigStrSize, 'x')
            + ">\n2015-03-23 05:48:31,000 done\n"
    };
    auto const result{convert_and_read_back(input, 64ULL * 1024ULL, 64ULL)};
    REQUIRE_FALSE(result.has_error());
    auto const& events{result.value()};
    REQUIRE(2ULL == events.size());
    REQUIRE((" <" + std::string(cBigStrSize, 'x') + ">\n") == events.at(0).message);
    REQUIRE(std::optional<std::string>{"2015-03-23 05:48:31,000"} == events.at(1).timestamp);
    REQUIRE(" done\n" == events.at(1).message);
}

TEST_CASE("log_converter_rejects_event_larger_than_max_buffer", "[clp_s][log_converter]") {
    TestOutputCleaner const test_output_cleaner{{std::string{cTestOutputDir}}};

    auto const input{std::string{"2015-03-23 05:48:30,122 <"} + std::string(4096, 'x') + ">\n"};
    auto const result{convert_and_read_back(input, 256, 256)};
    REQUIRE(result.has_error());
    REQUIRE(std::errc::result_out_of_range == result.error());
}

TEST_CASE("log_converter_parses_trailing_partial_buffer", "[clp_s][log_converter]") {
    TestOutputCleaner const test_output_cleaner{{std::string{cTestOutputDir}}};

    auto const input{
            std::string{"2015-03-23 05:48:30,122 first\n"}
            + "2015-03-23 05:48:31,000 last without newline"
    };
    auto const result{convert_and_read_back(input, 64ULL * 1024ULL, 48ULL)};
    REQUIRE_FALSE(result.has_error());
    auto const& events{result.value()};
    REQUIRE(2ULL == events.size());
    REQUIRE(" last without newline" == events.at(1).message);
}

TEST_CASE("log_converter_parses_multiline_event_across_refill", "[clp_s][log_converter]") {
    constexpr int cBigStrSize{256};
    TestOutputCleaner const test_output_cleaner{{std::string{cTestOutputDir}}};

    std::string input{"2015-03-23 05:48:30,122 start\n"};
    input += std::string(cBigStrSize, 'a').append("\n");
    input += std::string(cBigStrSize, 'b').append("\n");
    input += "2015-03-23 05:48:31,000 next\n";

    auto const result{convert_and_read_back(input, 64ULL * 1024ULL, 64ULL)};
    REQUIRE_FALSE(result.has_error());
    auto const& events{result.value()};
    REQUIRE(2ULL == events.size());
    REQUIRE(std::optional<std::string>{"2015-03-23 05:48:30,122"} == events.at(0).timestamp);
    REQUIRE((" start\n" + std::string(cBigStrSize, 'a') + "\n" + std::string(cBigStrSize, 'b')
             + "\n")
            == events.at(0).message);
    REQUIRE(" next\n" == events.at(1).message);
}

TEST_CASE("log_converter_parses_headerless_events", "[clp_s][log_converter]") {
    constexpr int cNumEvents{50};
    TestOutputCleaner const test_output_cleaner{{std::string{cTestOutputDir}}};

    std::string input;
    for (size_t idx{0}; idx < cNumEvents; ++idx) {
        input += "no timestamp event ";
        input += std::to_string(idx);
        input += "\n";
    }
    auto const result{convert_and_read_back(input, 64ULL * 1024ULL, 64ULL)};
    REQUIRE_FALSE(result.has_error());
    auto const& events{result.value()};
    REQUIRE(50ULL == events.size());
    for (auto const& event : events) {
        REQUIRE_FALSE(event.timestamp.has_value());
    }
    std::string reconstructed;
    for (auto const& event : events) {
        reconstructed += event.message;
    }
    REQUIRE(input == reconstructed);
}
}  // namespace clp_s::log_converter::test
