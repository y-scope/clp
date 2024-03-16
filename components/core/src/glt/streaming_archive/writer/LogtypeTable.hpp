#ifndef GLT_STREAMING_ARCHIVE_WRITER_LOGTYPETABLE_HPP
#define GLT_STREAMING_ARCHIVE_WRITER_LOGTYPETABLE_HPP

// C++ standard libraries
#include <vector>

// Project headers
#include "../../Defs.h"
#include "../../ErrorCode.hpp"
#include "../../PageAllocatedVector.hpp"

namespace glt::streaming_archive::writer {
/**
 * Class for writing a Logtype Table. A LogtypeTable is a container for all messages belonging to a
 * single logtype. The table is arranged in a column-orientated manner where each column represents
 * a variable column from all messages of the logtype, plus timestamp and file_id column
 */
class LogtypeTable {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "streaming_archive::writer::LogtypeTable operation failed";
        }
    };

    // Constructor
    /**
     * Initialize the logtype table for a logtype
     * with num_columns variables
     * @param timestamp
     * @param file_id
     * @param encoded_vars
     */
    LogtypeTable(size_t num_columns);

    /**
     * Writes the variable row into the LogtypeTable
     * @param timestamp
     * @param file_id
     * @param encoded_vars
     */
    void append_to_table(
            epochtime_t timestamp,
            file_id_t file_id,
            std::vector<encoded_variable_t> const& encoded_vars
    );

    size_t get_num_rows() const { return m_num_rows; }

    size_t get_num_columns() const { return m_num_columns; }

    std::vector<std::vector<encoded_variable_t>> const& get_variables() const {
        return m_variables;
    }

    std::vector<epochtime_t> const& get_timestamps() const { return m_timestamp; }

    std::vector<file_id_t> const& get_file_ids() const { return m_file_ids; }

private:
    // Variables
    size_t m_num_columns;
    size_t m_num_rows;
    std::vector<std::vector<encoded_variable_t>> m_variables;
    std::vector<epochtime_t> m_timestamp;
    std::vector<file_id_t> m_file_ids;
};
}  // namespace glt::streaming_archive::writer

#endif  // GLT_STREAMING_ARCHIVE_WRITER_LOGTYPETABLE_HPP
