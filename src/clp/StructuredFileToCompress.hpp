#ifndef CLP_STRUCTUREDFILETOCOMPRESS_HPP
#define CLP_STRUCTUREDFILETOCOMPRESS_HPP

// C++ standard libraries
#include <string>

// Project headers
#include "FileToCompress.hpp"

namespace clp {
    /**
     * Class to store data about a structured file to compress
     */
    class StructuredFileToCompress : public FileToCompress {
    public:
        // Constructors
        StructuredFileToCompress (const std::string& path, const std::string& path_for_compression, const std::string& timestamps_file_path,
                                  group_id_t group_id) : FileToCompress(path, path_for_compression, group_id), m_timestamps_file_path(timestamps_file_path) {}

        // Methods
        const std::string& get_timestamps_file_path () const { return m_timestamps_file_path; }

    private:
        // Variables
        std::string m_timestamps_file_path;
    };
}

#endif // CLP_STRUCTUREDFILETOCOMPRESS_HPP
