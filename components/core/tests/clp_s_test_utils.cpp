#include "clp_s_test_utils.hpp"

#include <filesystem>
#include <string>
#include <vector>

#include <catch2/catch.hpp>

#include "../src/clp_s/ArchiveWriter.hpp"
#include "../src/clp_s/InputConfig.hpp"
#include "../src/clp_s/JsonParser.hpp"

auto compress_archive(
        std::string const& file_path,
        std::string const& archive_directory,
        bool single_file_archive,
        bool structurize_arrays,
        clp_s::FileType file_type
) -> std::vector<clp_s::ArchiveStats> {
    constexpr auto cDefaultTargetEncodedSize{8ULL * 1024 * 1024 * 1024};  // 8 GiB
    constexpr auto cDefaultMaxDocumentSize{512ULL * 1024 * 1024};  // 512 MiB
    constexpr auto cDefaultMinTableSize{1ULL * 1024 * 1024};  // 1 MiB
    constexpr auto cDefaultCompressionLevel{3};
    constexpr auto cDefaultPrintArchiveStats{false};

    std::filesystem::create_directory(archive_directory);
    REQUIRE((std::filesystem::is_directory(archive_directory)));

    clp_s::JsonParserOption parser_option{};
    parser_option.input_paths.emplace_back(
            clp_s::Path{.source = clp_s::InputSource::Filesystem, .path = file_path}
    );
    parser_option.archives_dir = archive_directory;
    parser_option.target_encoded_size = cDefaultTargetEncodedSize;
    parser_option.max_document_size = cDefaultMaxDocumentSize;
    parser_option.min_table_size = cDefaultMinTableSize;
    parser_option.compression_level = cDefaultCompressionLevel;
    parser_option.print_archive_stats = cDefaultPrintArchiveStats;
    parser_option.structurize_arrays = structurize_arrays;
    parser_option.single_file_archive = single_file_archive;
    parser_option.input_file_type = file_type;

    clp_s::JsonParser parser{parser_option};
    std::vector<clp_s::ArchiveStats> archive_stats;
    if (clp_s::FileType::Json == file_type) {
        REQUIRE(parser.parse());
    } else if (clp_s::FileType::KeyValueIr == file_type) {
        REQUIRE(parser.parse_from_ir());
    } else {
        // This branch should be unreachable.
        REQUIRE(false);
    }
    REQUIRE_NOTHROW(archive_stats = parser.store());

    REQUIRE((false == std::filesystem::is_empty(archive_directory)));
    return archive_stats;
}
