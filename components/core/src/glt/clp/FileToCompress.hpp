#ifndef CLP_CLP_FILETOCOMPRESS_HPP
#define CLP_CLP_FILETOCOMPRESS_HPP

#include <string>

#include "../Defs.h"

namespace clp::clp {
/**
 * Class to store data about a file to compress
 */
class FileToCompress {
public:
    // Constructors
    FileToCompress(
            std::string const& path,
            std::string const& path_for_compression,
            group_id_t group_id
    )
            : m_path(path),
              m_path_for_compression(path_for_compression),
              m_group_id(group_id) {}

    // Methods
    std::string const& get_path() const { return m_path; }

    std::string const& get_path_for_compression() const { return m_path_for_compression; }

    group_id_t get_group_id() const { return m_group_id; }

private:
    // Variables
    std::string m_path;
    std::string m_path_for_compression;
    group_id_t m_group_id;
};
}  // namespace clp::clp

#endif  // CLP_CLP_FILETOCOMPRESS_HPP
