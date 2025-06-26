#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch.hpp>

#include "../src/clp_s/archive_constants.hpp"
#include "../src/clp_s/ArchiveReader.hpp"
#include "../src/clp_s/ColumnReader.hpp"
#include "../src/clp_s/InputConfig.hpp"
#include "../src/clp_s/SchemaReader.hpp"
#include "clp_s_test_utils.hpp"
#include "TestOutputCleaner.hpp"

constexpr std::string_view cTestDeltaEncodeOrderArchiveDirectory{"test-delta-encode-order-archive"};
constexpr std::string_view cTestDeltaEncodeOrderInputFileDirectory{"test_log_files"};
constexpr std::string_view cTestDeltaEncodeOrderInputFile{"test_simple_order.jsonl"};
constexpr size_t cNumEntries{3};

namespace {
/**
 * A simple implementation of `clp_s::FilterClass` that allows us to grab the underlying
 * `std::vector<BaseColumnReader*>` from a `SchemaReader`.
 */
class SimpleFilterClass : public clp_s::FilterClass {
public:
    void init(
            clp_s::SchemaReader* reader,
            std::vector<clp_s::BaseColumnReader*> const& column_readers
    ) override {
        m_column_readers = column_readers;
    }

    auto filter(uint64_t cur_message) -> bool override { return true; }

    auto get_column_readers() -> std::vector<clp_s::BaseColumnReader*> const& {
        return m_column_readers;
    }

private:
    std::vector<clp_s::BaseColumnReader*> m_column_readers;
};

auto get_test_input_path_relative_to_tests_dir() -> std::filesystem::path;
auto get_test_input_local_path() -> std::string;

auto get_test_input_path_relative_to_tests_dir() -> std::filesystem::path {
    return std::filesystem::path{cTestDeltaEncodeOrderInputFileDirectory}
           / cTestDeltaEncodeOrderInputFile;
}

auto get_test_input_local_path() -> std::string {
    std::filesystem::path const current_file_path{__FILE__};
    auto const tests_dir{current_file_path.parent_path()};
    return (tests_dir / get_test_input_path_relative_to_tests_dir()).string();
}
}  // namespace

TEST_CASE("clp-s-delta-encode-log-order", "[clp-s][delta-encode-log-order]") {
    auto start_index = GENERATE(0ULL, 1ULL, 2ULL);
    TestOutputCleaner const test_cleanup{{std::string{cTestDeltaEncodeOrderArchiveDirectory}}};

    REQUIRE_NOTHROW(compress_archive(
            get_test_input_local_path(),
            std::string{cTestDeltaEncodeOrderArchiveDirectory},
            true,
            false,
            clp_s::FileType::Json
    ));

    std::vector<clp_s::Path> archive_paths;
    REQUIRE(clp_s::get_input_archives_for_raw_path(
            std::string{cTestDeltaEncodeOrderArchiveDirectory},
            archive_paths
    ));
    REQUIRE(1 == archive_paths.size());

    clp_s::ArchiveReader archive_reader;
    REQUIRE_NOTHROW(archive_reader.open(archive_paths.back(), clp_s::NetworkAuthOption{}));
    REQUIRE_NOTHROW(archive_reader.read_dictionaries_and_metadata());
    REQUIRE_NOTHROW(archive_reader.open_packed_streams());
    auto mpt = archive_reader.get_schema_tree();
    auto log_event_idx_node_id = mpt->get_metadata_field_id(clp_s::constants::cLogEventIdxName);
    REQUIRE(-1 != log_event_idx_node_id);

    std::vector<std::shared_ptr<clp_s::SchemaReader>> schema_readers;
    REQUIRE_NOTHROW(schema_readers = archive_reader.read_all_tables());
    REQUIRE(1 == schema_readers.size());
    auto schema_reader = schema_readers.back();
    REQUIRE(cNumEntries == schema_reader->get_num_messages());

    SimpleFilterClass simple_filter_class;
    schema_reader->initialize_filter(&simple_filter_class);
    clp_s::BaseColumnReader* log_event_idx_reader{nullptr};
    for (auto* column_reader : simple_filter_class.get_column_readers()) {
        if (log_event_idx_node_id == column_reader->get_id()) {
            log_event_idx_reader = column_reader;
            break;
        }
    }
    REQUIRE(nullptr != log_event_idx_reader);
    REQUIRE(clp_s::NodeType::DeltaInteger == log_event_idx_reader->get_type());
    REQUIRE(nullptr != dynamic_cast<clp_s::DeltaEncodedInt64ColumnReader*>(log_event_idx_reader));

    // Test forwards and backwards seeks on `DeltaEncodedInt64ColumnReader`.
    size_t i{start_index};
    for (size_t num_iterations{0ULL}; num_iterations < cNumEntries; ++num_iterations) {
        int64_t val{};
        REQUIRE_NOTHROW(val = std::get<int64_t>(log_event_idx_reader->extract_value(i)));
        REQUIRE(val == static_cast<int64_t>(i));
        i = (i + 1) % cNumEntries;
    }
    REQUIRE_NOTHROW(archive_reader.close());
}
