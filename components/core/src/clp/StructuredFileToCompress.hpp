#ifndef CLP_STRUCTUREDFILETOCOMPRESS_HPP
#define CLP_STRUCTUREDFILETOCOMPRESS_HPP

#include <string>

#include "FileToCompress.hpp"

namespace clp {
/**
 * Class to store data about a structured file to compress
 */
class StructuredFileToCompress : public FileToCompress {
public:
    // Constructors
    StructuredFileToCompress(
            std::string const& path,
            std::string const& path_for_compression,
            std::string const& timestamps_file_path,
            group_id_t group_id
    )
            : FileToCompress(path, path_for_compression, group_id),
              m_timestamps_file_path(timestamps_file_path) {}

    // Methods
    std::string const& get_timestamps_file_path() const { return m_timestamps_file_path; }

private:
    // Variables
    std::string m_timestamps_file_path;
};
}  // namespace clp

#endif  // CLP_STRUCTUREDFILETOCOMPRESS_HPP
