#ifndef CLP_CLP_UTILS_HPP
#define CLP_CLP_UTILS_HPP

#include <filesystem>
#include <memory>
#include <string>

#include <boost/filesystem/path.hpp>

#include "../ErrorCode.hpp"
#include "../GlobalMetadataDB.hpp"
#include "../GlobalMetadataDBConfig.hpp"
#include "../TraceableException.hpp"
#include "FileToCompress.hpp"

namespace clp::clp {
// Types
class ClpOperationFailed : public TraceableException {
public:
    // Constructors
    ClpOperationFailed(ErrorCode error_code, char const* const filename, int line_number)
            : TraceableException(error_code, filename, line_number) {}

    // Methods
    [[nodiscard]] char const* what() const noexcept override { return "CLP operation failed"; }
};

// Methods
/**
 * Recursively finds all files and empty directories at the given path
 * @param path_prefix_to_remove
 * @param path
 * @param file_paths
 * @param empty_directory_paths
 * @return true on success, false otherwise
 */
bool find_all_files_and_empty_directories(
        boost::filesystem::path& path_prefix_to_remove,
        std::string const& path,
        std::vector<FileToCompress>& file_paths,
        std::vector<std::string>& empty_directory_paths
);

/**
 * Reads a list of input paths
 * @param list_path
 * @param paths
 * @return true on success, false otherwise
 */
bool read_input_paths(std::string const& list_path, std::vector<std::string>& paths);

/**
 * Removes the given prefix from the given path and cleans the path as follows:
 * - Removes redundant '.' and ".."
 * - Makes the path absolute
 * @param prefix_to_remove
 * @param path
 * @param path_without_prefix_string
 * @return false if the path didn't contain the prefix or it didn't contain anything besides the
 * prefix, true otherwise
 */
bool remove_prefix_and_clean_up_path(
        boost::filesystem::path const& prefix_to_remove,
        boost::filesystem::path const& path,
        std::string& path_without_prefix_string
);

/**
 * Validates that all paths in the given list exist
 * @param paths
 * @return true if they all exist, false otherwise
 */
bool validate_paths_exist(std::vector<std::string> const& paths);

/**
 * Chooses and initializes the relevant global metadata DB class based on the given config.
 * @param global_metadata_db_config
 * @param archives_dir
 * @return The relevant global metadata DB class.
 */
std::unique_ptr<GlobalMetadataDB> get_global_metadata_db(
        GlobalMetadataDBConfig const& global_metadata_db_config,
        std::filesystem::path const& archives_dir
);
}  // namespace clp::clp

#endif  // CLP_CLP_UTILS_HPP
