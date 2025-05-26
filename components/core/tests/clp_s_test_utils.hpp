#ifndef CLP_S_TEST_UTILS_HPP
#define CLP_S_TEST_UTILS_HPP
#include <string>

#include "../src/clp_s/CommandLineArguments.hpp"

/**
 * Compresses a file into an archive directory according to a given set of configuration options.
 *
 * This helper uses `REQUIRE...` statements to assert that compression was successful.
 *
 * @param file_path
 * @param archive_directory
 * @param single_file_archive
 * @param structurize_arrays
 * @param file_type
 */
void compress_archive(
        std::string const& file_path,
        std::string const& archive_directory,
        bool single_file_archive,
        bool structurize_arrays,
        clp_s::CommandLineArguments::FileType file_type
);
#endif  // CLP_S_TEST_UTILS_HPP
