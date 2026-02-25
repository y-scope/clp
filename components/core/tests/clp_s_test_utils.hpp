#ifndef CLP_S_TEST_UTILS_HPP
#define CLP_S_TEST_UTILS_HPP

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "../src/clp_s/ArchiveWriter.hpp"
#include "../src/clp_s/InputConfig.hpp"

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
 * @return Statistics for every compressed archive.
 */
[[nodiscard]] auto compress_archive(
        std::string const& file_path,
        std::string const& archive_directory,
        std::optional<std::string> timestamp_key,
        bool retain_float_format,
        bool single_file_archive,
        bool structurize_arrays
) -> std::vector<clp_s::ArchiveStats>;

/**
 * @param test_input_path The name of the test input file.
 * @return The absolute path to the test input file within the test log directory.
 */
[[nodiscard]] auto get_test_input_local_path(std::string_view test_input_path) -> std::string;
#endif  // CLP_S_TEST_UTILS_HPP
