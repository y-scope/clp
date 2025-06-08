#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>

#include <catch2/catch.hpp>
#include <fmt/format.h>
#include <msgpack.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "../src/clp/ffi/ir_stream/protocol_constants.hpp"
#include "../src/clp/ffi/ir_stream/Serializer.hpp"
#include "../src/clp/FileReader.hpp"
#include "../src/clp/FileWriter.hpp"
#include "../src/clp/ir/types.hpp"
#include "../src/clp/streaming_compression/zstd/Compressor.hpp"
#include "../src/clp/type_utils.hpp"
#include "../src/clp_s/archive_constants.hpp"
#include "../src/clp_s/ArchiveReader.hpp"
#include "../src/clp_s/InputConfig.hpp"
#include "clp_s_test_utils.hpp"
#include "TestOutputCleaner.hpp"

constexpr std::string_view cTestRangeIndexArchiveDirectory{"test-range-index-archive"};
constexpr std::string_view cTestRangeIndexIRDirectory{"test-range-index-ir"};
constexpr std::string_view cTestRangeIndexInputFileDirectory{"test_log_files"};
constexpr std::string_view cTestRangeIndexInputFile{"test_no_floats_sorted.jsonl"};
constexpr std::string_view cTestRangeIndexIRInputFile{"test_no_floats_sorted.ir"};
constexpr std::string_view cTestRangeIndexIRMetadataKey{"test_key"};
constexpr std::string_view cTestRangeIndexIRMetadataValue{"test_value"};

namespace {
auto get_test_input_path_relative_to_tests_dir() -> std::filesystem::path;
auto get_test_input_local_path() -> std::string;
void serialize_record(
        nlohmann::json const& auto_gen,
        nlohmann::json const& user_gen,
        clp::ffi::ir_stream::Serializer<clp::ir::eight_byte_encoded_variable_t>& serializer
);
void generate_ir();
void check_archive_metadata(bool from_ir);

auto get_test_input_path_relative_to_tests_dir() -> std::filesystem::path {
    return std::filesystem::path{cTestRangeIndexInputFileDirectory} / cTestRangeIndexInputFile;
}

auto get_test_input_local_path() -> std::string {
    std::filesystem::path const current_file_path{__FILE__};
    auto const tests_dir{current_file_path.parent_path()};
    return (tests_dir / get_test_input_path_relative_to_tests_dir()).string();
}

auto get_ir_test_input_relative_path() -> std::string {
    return (std::filesystem::path{cTestRangeIndexIRDirectory} / cTestRangeIndexIRInputFile)
            .string();
}

void serialize_record(
        nlohmann::json const& auto_gen,
        nlohmann::json const& user_gen,
        clp::ffi::ir_stream::Serializer<clp::ir::eight_byte_encoded_variable_t>& serializer
) {
    auto const auto_gen_bytes{nlohmann::json::to_msgpack(auto_gen)};
    auto const user_gen_bytes{nlohmann::json::to_msgpack(user_gen)};
    auto const auto_gen_handle{msgpack::unpack(
            clp::size_checked_pointer_cast<char const>(auto_gen_bytes.data()),
            auto_gen_bytes.size()
    )};
    auto const user_gen_handle{msgpack::unpack(
            clp::size_checked_pointer_cast<char const>(user_gen_bytes.data()),
            user_gen_bytes.size()
    )};
    auto const auto_gen_obj{auto_gen_handle.get()};
    auto const user_gen_obj{user_gen_handle.get()};
    REQUIRE(msgpack::type::MAP == auto_gen_obj.type);
    REQUIRE(msgpack::type::MAP == user_gen_obj.type);
    REQUIRE(serializer.serialize_msgpack_map(auto_gen_obj.via.map, user_gen_obj.via.map));
}

void generate_ir() {
    std::filesystem::create_directory(cTestRangeIndexIRDirectory);
    REQUIRE(std::filesystem::is_directory(cTestRangeIndexIRDirectory));

    nlohmann::json ir_metadata = {{cTestRangeIndexIRMetadataKey, cTestRangeIndexIRMetadataValue}};

    auto result{clp::ffi::ir_stream::Serializer<clp::ir::eight_byte_encoded_variable_t>::create(
            ir_metadata
    )};
    REQUIRE(false == result.has_error());
    auto& serializer = result.value();

    auto empty_object = nlohmann::json::parse("{}");
    clp::FileReader reader(get_test_input_local_path());
    std::string line;
    while (clp::ErrorCode::ErrorCode_Success
           == reader.try_read_to_delimiter('\n', false, false, line))
    {
        auto json_line = nlohmann::json::parse(line);
        serialize_record(empty_object, json_line, serializer);
    }

    clp::FileWriter writer;
    REQUIRE_NOTHROW(writer.open(
            get_ir_test_input_relative_path(),
            clp::FileWriter::OpenMode::CREATE_FOR_WRITING
    ));
    clp::streaming_compression::zstd::Compressor compressor;
    compressor.open(writer);

    auto const eof_packet{clp::ffi::ir_stream::cProtocol::Eof};
    auto const ir_buf{serializer.get_ir_buf_view()};
    compressor.write(clp::size_checked_pointer_cast<char const>(ir_buf.data()), ir_buf.size());
    compressor.write(clp::size_checked_pointer_cast<char const>(&eof_packet), sizeof(eof_packet));
    compressor.close();
    writer.close();
}

void check_archive_metadata(bool from_ir) {
    clp_s::ArchiveReader archive_reader;
    auto const expected_input_path{
            from_ir ? get_ir_test_input_relative_path() : get_test_input_local_path()
    };
    for (auto const& entry : std::filesystem::directory_iterator(cTestRangeIndexArchiveDirectory)) {
        clp_s::Path archive_path{
                .source{clp_s::InputSource::Filesystem},
                .path{entry.path().string()}
        };
        REQUIRE_NOTHROW(archive_reader.open(archive_path, clp_s::NetworkAuthOption{}));
        auto const& range_index = archive_reader.get_range_index();
        REQUIRE(1ULL == range_index.size());
        auto const& range_index_entry = range_index.front();
        REQUIRE(0ULL == range_index_entry.start_index);
        REQUIRE(4ULL == range_index_entry.end_index);
        auto const& metadata_fields = range_index_entry.fields;
        REQUIRE(metadata_fields.contains(clp_s::constants::range_index::cArchiveCreatorId));
        REQUIRE(metadata_fields.at(clp_s::constants::range_index::cArchiveCreatorId).is_string());
        REQUIRE(false
                == metadata_fields.at(clp_s::constants::range_index::cArchiveCreatorId)
                           .template get<std::string>()
                           .empty());
        REQUIRE(metadata_fields.contains(clp_s::constants::range_index::cFilename));
        REQUIRE(metadata_fields.at(clp_s::constants::range_index::cFilename).is_string());
        REQUIRE(expected_input_path
                == metadata_fields.at(clp_s::constants::range_index::cFilename)
                           .template get<std::string>());
        REQUIRE(metadata_fields.contains(clp_s::constants::range_index::cFileSplitNumber));
        REQUIRE(metadata_fields.at(clp_s::constants::range_index::cFileSplitNumber)
                        .is_number_integer());
        REQUIRE(0ULL
                == metadata_fields.at(clp_s::constants::range_index::cFileSplitNumber)
                           .template get<size_t>());
        if (from_ir) {
            REQUIRE(metadata_fields.contains(cTestRangeIndexIRMetadataKey));
            REQUIRE(metadata_fields.at(cTestRangeIndexIRMetadataKey).is_string());
            REQUIRE(
                    cTestRangeIndexIRMetadataValue
                    == metadata_fields.at(cTestRangeIndexIRMetadataKey).template get<std::string>()
            );
        }
        REQUIRE_NOTHROW(archive_reader.close());
    }
}
}  // namespace

TEST_CASE("clp-s-range-index", "[clp-s][range-index]") {
    auto single_file_archive = GENERATE(true, false);
    auto from_ir = GENERATE(true, false);

    TestOutputCleaner const test_cleanup{
            {std::string{cTestRangeIndexArchiveDirectory}, std::string{cTestRangeIndexIRDirectory}}
    };

    auto input_file{get_test_input_local_path()};
    auto input_file_type{clp_s::FileType::Json};
    if (from_ir) {
        generate_ir();
        input_file = get_ir_test_input_relative_path();
        input_file_type = clp_s::FileType::KeyValueIr;
    }
    REQUIRE_NOTHROW(compress_archive(
            input_file,
            std::string{cTestRangeIndexArchiveDirectory},
            single_file_archive,
            false,
            input_file_type
    ));
    check_archive_metadata(from_ir);
}
