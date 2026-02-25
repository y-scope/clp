#ifndef CLP_S_INPUTCONFIG_HPP
#define CLP_S_INPUTCONFIG_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "../clp/ReaderInterface.hpp"

namespace clp_s {
// Constants used for input configuration
constexpr char cAwsAccessKeyIdEnvVar[] = "AWS_ACCESS_KEY_ID";
constexpr char cAwsSecretAccessKeyEnvVar[] = "AWS_SECRET_ACCESS_KEY";
constexpr char cAwsSessionTokenEnvVar[] = "AWS_SESSION_TOKEN";

/**
 * Enum class defining various file types.
 */
enum class FileType : uint8_t {
    Json = 0,
    KeyValueIr,
    LogText,
    Zstd,
    Unknown
};

/**
 * Enum class defining the source of a resource.
 */
enum class InputSource : uint8_t {
    Filesystem,
    Network
};

/**
 * Enum class defining the authentication method required for accessing a resource.
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
 * Struct representing a resource path with its source type.
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
 * Removes a prefix from a filesystem path.
 * @param path
 * @param prefix
 * @return An option containing the path with the prefix removed on success, or an empty option on
 * failure.
 */
[[nodiscard]] auto remove_path_prefix(std::string_view path, std::string_view prefix)
        -> std::optional<std::string>;

/**
 * Recursively collects all file paths from the given raw path, including the path itself.
 * @param path
 * @param files Returned paths
 * @return true on success, false otherwise
 */
auto get_input_files_for_raw_path(std::string_view const path, std::vector<Path>& files) -> bool;

/**
 * Recursively collects all file paths that are children of the the given Path, including the Path
 * itself.
 * @param path
 * @param files Returned paths
 * @return true on success, false otherwise
 */
[[nodiscard]] auto get_input_files_for_path(Path const& path, std::vector<Path>& files) -> bool;

/**
 * Collects all archives that are children of the given raw path, including the path itself.
 * @param path
 * @param archives Returned archives
 * @return true on success, false otherwise
 */
[[nodiscard]] auto
get_input_archives_for_raw_path(std::string_view const path, std::vector<Path>& archives) -> bool;

/**
 * Collects all archives from the given Path, including the Path itself.
 * @param path
 * @param archives Returned archives
 * @return true on success, false otherwise
 */
[[nodiscard]] auto get_input_archives_for_path(Path const& path, std::vector<Path>& archives)
        -> bool;

/**
 * Determines the archive id of an archive based on the archive path.
 * @param path
 * @param archive_id Returned archive id
 * @return true on success, false otherwise
 */
[[nodiscard]] auto get_archive_id_from_path(Path const& archive_path, std::string& archive_id)
        -> bool;

/**
 * Tries to open a clp::ReaderInterface using the given Path and NetworkAuthOption.
 * @param path
 * @param network_auth
 * @return the opened clp::ReaderInterface or nullptr on error
 */
[[nodiscard]] auto try_create_reader(Path const& path, NetworkAuthOption const& network_auth)
        -> std::shared_ptr<clp::ReaderInterface>;

/**
 * Tries to deduce the underlying file-type of the file opened by `reader`, and returns a
 * (potentially new) reader for underlying JSON or KV-IR content by unwrapping layers of
 * compression.
 * @param reader
 * @return A vector of all created `clp::ReaderInterface`s, where the last entry in the vector is
 * open for reading content of the type described by the element in the pair. When the content type
 * cannot be deduced, we return an empty vector and `FileType::Unknown`.
 */
[[nodiscard]] auto try_deduce_reader_type(std::shared_ptr<clp::ReaderInterface> reader)
        -> std::pair<std::vector<std::shared_ptr<clp::ReaderInterface>>, FileType>;

/**
 * Closes all readers in a vector of nested readers, starting from the last reader.
 * @param readers
 */
void close_nested_readers(std::vector<std::shared_ptr<clp::ReaderInterface>> const& readers);
}  // namespace clp_s

#endif  // CLP_S_INPUTCONFIG_HPP
