#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "../src/clp/FileReader.hpp"
#include "../src/clp/ReaderInterface.hpp"
#include "../src/clp/ReadOnlyMemoryMappedFile.hpp"

namespace {
/**
 * Reads all content from a reader.
 * @param reader
 * @return The content.
 */
[[nodiscard]] auto read_content(clp::ReaderInterface& reader) -> std::vector<char>;

[[nodiscard]] auto get_test_dir() -> std::filesystem::path;

auto read_content(clp::ReaderInterface& reader) -> std::vector<char> {
    constexpr size_t cBufferSize{4096};
    std::array<char, cBufferSize> read_buf{};
    std::vector<char> buf;
    for (bool has_more_content{true}; has_more_content;) {
        size_t num_bytes_read{};
        has_more_content = reader.read(read_buf.data(), read_buf.size(), num_bytes_read);
        buf.insert(buf.cend(), read_buf.cbegin(), read_buf.cbegin() + num_bytes_read);
    }
    return buf;
}

auto get_test_dir() -> std::filesystem::path {
    std::filesystem::path const current_file_path{__FILE__};
    return current_file_path.parent_path();
}
}  // namespace

TEST_CASE("memory_mapped_file_view_basic", "[ReadOnlyMemoryMappedFile]") {
    auto const test_input_path{
            get_test_dir() / std::filesystem::path{"test_network_reader_src"} / "random.log"
    };
    clp::FileReader file_reader{test_input_path.string()};
    auto const expected{read_content(file_reader)};

    auto result{clp::ReadOnlyMemoryMappedFile::create(test_input_path.string())};
    REQUIRE_FALSE(result.has_error());
    auto const mmap_file{std::move(result.value())};

    auto const view{mmap_file.get_view()};
    REQUIRE((view.size() == expected.size()));
    REQUIRE(std::equal(view.begin(), view.end(), expected.cbegin()));
}

TEST_CASE("memory_mapped_file_view_empty", "[ReadOnlyMemoryMappedFile]") {
    auto const test_input_path{
            get_test_dir() / std::filesystem::path{"test_schema_files"} / "empty_schema.txt"
    };

    auto result{clp::ReadOnlyMemoryMappedFile::create(test_input_path.string())};
    REQUIRE_FALSE(result.has_error());
    auto const mmap_file{std::move(result.value())};

    auto const view{mmap_file.get_view()};
    REQUIRE(view.empty());
}

