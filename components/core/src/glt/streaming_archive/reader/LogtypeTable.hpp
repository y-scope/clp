#ifndef GLT_STREAMING_ARCHIVE_READER_LOGTYPETABLE_HPP
#define GLT_STREAMING_ARCHIVE_READER_LOGTYPETABLE_HPP

// C++ libraries
#include <vector>

// spdlog
#include <spdlog/spdlog.h>

// Project headers
#include "../../Defs.h"
#include "../../ErrorCode.hpp"
#include "../../streaming_compression/passthrough/Decompressor.hpp"
#include "../../streaming_compression/zstd/Decompressor.hpp"
#include "LogtypeMetadata.hpp"
#include "Message.hpp"

namespace glt::streaming_archive::reader {

/* this class is supposed to handle reading from a variable segment
 */

// Types
class OperationFailed : public TraceableException {
public:
    // Constructors
    OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
            : TraceableException(error_code, filename, line_number) {}

    // Methods
    char const* what() const noexcept override { return "LibarchiveFileReader operation failed"; }
};

class LogtypeTable {
public:
    LogtypeTable() : m_read_buffer_ptr(nullptr), m_is_open(false) {}

    void open(char const* buffer, LogtypeMetadata const& metadata);
    void open_and_load_all(char const* buffer, LogtypeMetadata const& metadata);

    void close();

    bool is_open() const { return m_is_open; }

    size_t get_num_row() const { return m_num_row; }

    size_t get_num_column() const { return m_num_columns; }

    /**
     * Get next row in the loaded 2D variable columns and load timestamp, file_id and variables into
     * the msg
     * @param msg
     * @return
     */
    bool get_next_message(Message& msg);

    void get_next_row(std::vector<encoded_variable_t>& vars, size_t var_ix_begin, size_t var_ix_end)
            const;
    /**
     *
     */
    bool peek_next_ts(epochtime_t& ts);

    void skip_row();

    void load_timestamp();
    void load_variable_columns(size_t var_ix_begin, size_t var_ix_end);
    void load_remaining_data_into_vec(
            std::vector<epochtime_t>& ts,
            std::vector<file_id_t>& id,
            std::vector<encoded_variable_t>& vars,
            std::vector<size_t> const& potential_matched_row
    );

    /**
     * Get row in the loaded 2D variable columns with row_index = offset
     * @param msg
     * @return
     */
    void get_message_at_offset(size_t offset, Message& msg);

    epochtime_t get_timestamp_at_offset(size_t offset);

    /**
     * Open and load the 2D variable columns starting at buffer with compressed_size bytes
     * @param buffer
     * @param compressed_size
     */
    void load_all();

private:
    size_t m_current_row;
    size_t m_num_row;
    size_t m_num_columns;

    bool m_is_open;

    std::unique_ptr<char[]> m_read_buffer;
    // helper pointer to avoid get() everytime
    char* m_read_buffer_ptr;
    size_t m_buffer_size;

    char const* m_file_offset;
    LogtypeMetadata m_metadata;

    std::vector<bool> m_column_loaded;
    bool m_ts_loaded;

    std::vector<encoded_variable_t> m_timestamps;
    std::vector<file_id_t> m_file_ids;
    // for this data structure, m_column_based_variables[i] means all data at i th column
    // m_column_based_variables[i][j] means j th row at the i th column
    std::vector<encoded_variable_t> m_column_based_variables;

#if USE_PASSTHROUGH_COMPRESSION
    streaming_compression::passthrough::Decompressor m_decompressor;
#elif USE_ZSTD_COMPRESSION
    streaming_compression::zstd::Decompressor m_decompressor;
#else
    static_assert(false, "Unsupported compression mode.");
#endif

    void load_column(size_t column_ix);

    void load_ts_into_vec(
            std::vector<epochtime_t>& ts,
            std::vector<size_t> const& potential_matched_row
    );

    void load_file_id_into_vec(
            std::vector<file_id_t>& id,
            std::vector<size_t> const& potential_matched_row
    );

    void load_vars_into_vec(
            std::vector<encoded_variable_t>& vars,
            std::vector<size_t> const& potential_matched_row
    );
};
}  // namespace glt::streaming_archive::reader

#endif  // GLT_STREAMING_ARCHIVE_READER_LOGTYPETABLE_HPP
