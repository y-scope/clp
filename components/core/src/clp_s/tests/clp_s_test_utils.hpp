#ifndef CLP_S_TEST_UTILS_HPP
#define CLP_S_TEST_UTILS_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <clp_s/ArchiveWriter.hpp>

/**
 * Compresses a file into an archive directory according to a given set of configuration options.
 *
 * This helper uses `REQUIRE...` statements to assert that compression was successful.
 *
 * @param file_path
 * @param archive_directory
 * @param timestamp_key
 * @param retain_float_format
 * @param single_file_archive
 * @param structurize_arrays
 * @param target_encoded_size The archive split threshold, or the default threshold if empty.
 * @return Statistics for every compressed archive.
 */
[[nodiscard]] auto compress_archive(
        std::string const& file_path,
        std::string const& archive_directory,
        std::optional<std::string> timestamp_key,
        bool retain_float_format,
        bool single_file_archive,
        bool structurize_arrays,
        std::optional<std::size_t> target_encoded_size = std::nullopt
) -> std::vector<clp_s::ArchiveStats>;
#endif  // CLP_S_TEST_UTILS_HPP
