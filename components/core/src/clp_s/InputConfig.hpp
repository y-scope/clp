#ifndef CLP_S_INPUTCONFIG_HPP
#define CLP_S_INPUTCONFIG_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace clp_s {
// Constants used for input configuration
constexpr char cAwsAccessKeyIdEnvVar[] = "AWS_ACCESS_KEY_ID";
constexpr char cAwsSecretAccessKeyEnvVar[] = "AWS_SECRET_ACCESS_KEY";

/**
 * Enum class definining the source of some resource.
 */
enum class InputSource : uint8_t {
    Filesystem,
    Network
};

/**
 * Enum class defining the auth that needs to be performed to access some resource.
 */
enum class AuthMethod : uint8_t {
    None,
    S3PresignedUrlV4
};

/**
 * Struct encapsulating information needed to authenticate network requests.
 */
struct NetworkAuthOption {
    AuthMethod method{AuthMethod::None};
};

/**
 * Struct describing a path to some resource as well as its source.
 */
struct Path {
    InputSource source{InputSource::Filesystem};
    std::string path;
};

/**
 * Determines the input source for a given raw path or url.
 * @param path
 * @return the InputSource for the given path
 */
[[nodiscard]] auto get_source_for_path(std::string_view const path) -> InputSource;

/**
 * Determines the input source for a given raw path or url and converts the path into a Path object.
 * @param path
 * @return a Path object representing the raw path or url
 */
[[nodiscard]] auto get_path_object_for_raw_path(std::string_view const path) -> Path;

/**
 * Recursively records all file paths from the given raw path, including the path itself.
 * @param path
 * @param files Returned paths
 * @return true on success, false otherwise
 */
auto get_input_files_for_raw_path(std::string_view const path, std::vector<Path>& files) -> bool;

/**
 * Recursively records all file paths that are children of the the given Path, including the Path
 * itself.
 * @param option
 * @param files Returned paths
 * @return true on success, false otherwise
 */
[[nodiscard]] auto get_input_files_for_path(Path const& path, std::vector<Path>& files) -> bool;

/**
 * Records all archives that are children of the given raw path, including the path itself.
 * @param path
 * @param archives Returned archives
 * @return true on success, false otherwise
 */
[[nodiscard]] auto
get_input_archives_for_raw_path(std::string_view const path, std::vector<Path>& archives) -> bool;

/**
 * Records all archives from the given Path, including the Path itself.
 * @param path
 * @param archives Returned archives
 * @return true on success, false otherwise
 */
[[nodiscard]] auto
get_input_archives_for_path(Path const& path, std::vector<Path>& archives) -> bool;

/**
 * Determines the archive id of a given archive based on a path to that archive.
 * @param path
 * @param archive_id Returned archive id
 * @return true on success, false otherwise
 */
[[nodiscard]] auto
get_archive_id_from_path(Path const& archive_path, std::string& archive_id) -> bool;
}  // namespace clp_s

#endif  // CLP_S_INPUTCONFIG_HPP
