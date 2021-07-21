#ifndef CLP_FILETOCOMPRESS_HPP
#define CLP_FILETOCOMPRESS_HPP

// C++ standard libraries
#include <string>

// Project headers
#include "../Defs.h"

namespace clp {
    /**
     * Class to store data about a file to compress
     */
    class FileToCompress {
    public:
        // Constructors
        FileToCompress (const std::string& path, const std::string& path_for_compression, group_id_t group_id) : m_path(path), m_path_for_compression(
                path_for_compression), m_group_id(group_id) {}

        // Methods
        const std::string& get_path () const { return m_path; }
        const std::string& get_path_for_compression () const { return m_path_for_compression; }
        group_id_t get_group_id () const { return m_group_id; }

    private:
        // Variables
        std::string m_path;
        std::string m_path_for_compression;
        group_id_t m_group_id;
    };
}

#endif // CLP_FILETOCOMPRESS_HPP
