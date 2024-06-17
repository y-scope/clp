#include <cstddef>
#include <filesystem>
#include <string_view>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/FileReader.hpp"
#include "../src/clp/MemoryMappedFileView.hpp"
#include "../src/clp/ReaderInterface.hpp"

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
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
    char read_buf[cBufferSize]{};
    std::vector<char> buf;
    for (bool has_more_content{true}; has_more_content;) {
        size_t num_bytes_read{};
        has_more_content = reader.read(static_cast<char*>(read_buf), cBufferSize, num_bytes_read);
        std::string_view const view{static_cast<char*>(read_buf), num_bytes_read};
        buf.insert(buf.cend(), view.cbegin(), view.cend());
    }
    return buf;
}

auto get_test_dir() -> std::filesystem::path {
    std::filesystem::path const current_file_path{__FILE__};
    return current_file_path.parent_path();
}
}  // namespace

TEST_CASE("memory_mapped_file_view_basic", "[MemoryMappedFileView]") {
    auto const test_input_path{
            get_test_dir() / std::filesystem::path{"test_network_reader_src"} / "random.log"
    };
    clp::FileReader file_reader;
    file_reader.open(test_input_path.string());
    auto const expected{read_content(file_reader)};
    file_reader.close();

    clp::MemoryMappedFileView const mmap_file{test_input_path.string()};
    auto const view{mmap_file.get_view()};
    std::vector<char> actual;
    actual.insert(actual.cend(), view.begin(), view.end());
    REQUIRE((expected == actual));
}

TEST_CASE("memory_mapped_file_view_empty", "[MemoryMappedFileView]") {
    auto const test_input_path{
            get_test_dir() / std::filesystem::path{"test_schema_files"} / "empty_schema.txt"
    };

    clp::MemoryMappedFileView const mmap_file{test_input_path.string()};
    auto const view{mmap_file.get_view()};
    REQUIRE(view.empty());
}
