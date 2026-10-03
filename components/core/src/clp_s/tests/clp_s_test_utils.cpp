#include "clp_s_test_utils.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <clp_s/ArchiveWriter.hpp>
#include <clp_s/CommandLineArguments.hpp>
#include <clp_s/InputConfig.hpp>
#include <clp_s/JsonParser.hpp>

auto compress_archive(
        std::string const& file_path,
        std::string const& archive_directory,
        std::optional<std::string> timestamp_key,
        bool retain_float_format,
        bool single_file_archive,
        bool structurize_arrays,
        std::optional<std::filesystem::path> parsing_spec_path
) -> std::vector<clp_s::ArchiveStats> {
    constexpr auto cDefaultTargetEncodedSize{8ULL * 1024 * 1024 * 1024};  // 8 GiB
    constexpr auto cDefaultMaxDocumentSize{512ULL * 1024 * 1024};  // 512 MiB
    constexpr auto cDefaultMinTableSize{1ULL * 1024 * 1024};  // 1 MiB
    constexpr auto cDefaultCompressionLevel{3};
    constexpr auto cDefaultPrintArchiveStats{false};

    std::filesystem::create_directory(archive_directory);
    REQUIRE((std::filesystem::is_directory(archive_directory)));

    clp_s::JsonParserOption parser_option{};
    parser_option.input_paths_and_canonical_filenames.emplace_back(
            clp_s::Path{.source = clp_s::InputSource::Filesystem, .path = file_path},
            file_path
    );
    parser_option.archives_dir = archive_directory;
    parser_option.target_encoded_size = cDefaultTargetEncodedSize;
    parser_option.max_document_size = cDefaultMaxDocumentSize;
    parser_option.min_table_size = cDefaultMinTableSize;
    parser_option.compression_level = cDefaultCompressionLevel;
    parser_option.print_archive_stats = cDefaultPrintArchiveStats;
    parser_option.retain_float_format = retain_float_format;
    parser_option.structurize_arrays = structurize_arrays;
    parser_option.single_file_archive = single_file_archive;
    if (timestamp_key.has_value()) {
        parser_option.timestamp_key = std::move(timestamp_key.value());
    }
    if (parsing_spec_path.has_value()) {
        parser_option.experimental = clp_s::CommandLineArguments::ExperimentalOptions{
                .parsing_spec_path = clp_s::Path{
                        .source = clp_s::InputSource::Filesystem,
                        .path = parsing_spec_path.value().string()
                }
        };
    }

    clp_s::JsonParser parser{parser_option};
    std::vector<clp_s::ArchiveStats> archive_stats;
    REQUIRE(parser.ingest());
    REQUIRE_NOTHROW(archive_stats = parser.store());

    REQUIRE((false == std::filesystem::is_empty(archive_directory)));
    return archive_stats;
}

auto get_heuristic_parsing_spec_path() -> std::filesystem::path {
    auto const clp_s_dir{std::filesystem::path{__FILE__}.parent_path().parent_path()};
    auto const components_dir{clp_s_dir.parent_path().parent_path().parent_path()};
    return (components_dir / "package-template" / "src" / "etc" / "parsing-spec.template.txt")
            .string();
}
