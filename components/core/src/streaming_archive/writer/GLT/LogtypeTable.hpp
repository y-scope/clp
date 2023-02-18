#ifndef STREAMING_ARCHIVE_WRITER_GLT_LOGTYPETABLE_HPP
#define STREAMING_ARCHIVE_WRITER_GLT_LOGTYPETABLE_HPP

// C++ standard libraries
#include <vector>

// Project headers
#include "../../../Defs.h"
#include "../../../ErrorCode.hpp"
#include "../../../PageAllocatedVector.hpp"

// This class caches all variable rows of a specific logtype in the memory for later writing.
// During compression, both File and CompressedStreamOnDisk own a LogtypeTable for each logtype.
// The variable row is first stored into the File's VariableSegmentWriter. Upon file closure, the
// Variable row will then be copied from File to CompressedStreamOnDisk. The design is because while processing a file,
// we don't know if the file has timestamps hence we don't know which segment to insert variable rows into.
// Idea: maybe I want to create two classes. the wrap around the LogtypeTable class, for File adn segment Class
namespace streaming_archive::writer {
    class LogtypeTable {
    public:

        LogtypeTable (size_t num_columns);

        /**
         * Writes the variable row into the LogtypeTable
         * @param timestamp
         * @param file_id
         * @param encoded_vars
         */
        void append_to_table (epochtime_t timestamp, file_id_t file_id,
                              const std::vector<encoded_variable_t>& encoded_vars);

        size_t get_num_rows () const { return m_num_rows; }

        size_t get_num_columns () const { return m_num_columns; }

        const std::vector<std::vector<encoded_variable_t>>& get_variables () const { return m_variables; }

        const std::vector<epochtime_t>& get_timestamps () const { return m_timestamp; }

        const std::vector<file_id_t>& get_file_ids () const { return m_file_ids; }

    private:
        // Number of columns for the given logtype
        size_t m_num_columns;
        size_t m_num_rows;
        std::vector<std::vector<encoded_variable_t>> m_variables;
        std::vector<epochtime_t> m_timestamp;
        std::vector<file_id_t> m_file_ids;

    };
}

#endif //STREAMING_ARCHIVE_WRITER_GLT_LOGTYPETABLE_HPP
