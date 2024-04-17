#include "LogtypeTable.hpp"

// Boost libraries
#include <boost/filesystem.hpp>

namespace glt::streaming_archive::reader {

void LogtypeTable::open_and_load_all(char const* buffer, LogtypeMetadata const& metadata) {
    open(buffer, metadata);
    load_all();
}

void LogtypeTable::load_all() {
    // now we can start to read the variables. first figure out how many rows are there
    size_t num_bytes_read = 0;
    char const* ts_start = m_file_offset + m_metadata.ts_offset;
    m_decompressor.open(ts_start, m_metadata.ts_size);
    // read out the time stamp
    m_timestamps.resize(m_num_row);
    m_decompressor.try_read(m_read_buffer_ptr, m_buffer_size, num_bytes_read);
    if (num_bytes_read != m_buffer_size) {
        SPDLOG_ERROR(
                "Wrong number of Bytes read: Expect: {}, Got: {}",
                m_buffer_size,
                num_bytes_read
        );
        throw ErrorCode_Failure;
    }
    m_decompressor.close();
    epochtime_t* converted_timestamp_ptr = reinterpret_cast<epochtime_t*>(m_read_buffer_ptr);
    for (size_t row_ix = 0; row_ix < m_num_row; row_ix++) {
        m_timestamps[row_ix] = converted_timestamp_ptr[row_ix];
    }

    char const* filed_id_start = m_file_offset + m_metadata.file_id_offset;
    m_decompressor.open(filed_id_start, m_metadata.file_id_size);

    m_file_ids.resize(m_num_row);
    size_t read_size = sizeof(file_id_t) * m_num_row;
    m_decompressor.try_read(m_read_buffer_ptr, read_size, num_bytes_read);
    if (num_bytes_read != read_size) {
        SPDLOG_ERROR(
                "Wrong number of Bytes read: Expect: {}, Got: {}",
                m_buffer_size,
                num_bytes_read
        );
        throw ErrorCode_Failure;
    }
    m_decompressor.close();
    file_id_t* converted_file_id_ptr = reinterpret_cast<file_id_t*>(m_read_buffer_ptr);
    for (size_t row_ix = 0; row_ix < m_num_row; row_ix++) {
        m_file_ids[row_ix] = converted_file_id_ptr[row_ix];
    }

    m_column_based_variables.resize(m_num_row * m_num_columns);
    for (int column_ix = 0; column_ix < m_num_columns; column_ix++) {
        char const* var_start = m_file_offset + m_metadata.column_offset[column_ix];
        m_decompressor.open(var_start, m_metadata.column_size[column_ix]);
        m_decompressor.try_read(m_read_buffer_ptr, m_buffer_size, num_bytes_read);
        if (num_bytes_read != m_buffer_size) {
            SPDLOG_ERROR(
                    "Wrong number of Bytes read: Expect: {}, Got: {}",
                    m_buffer_size,
                    num_bytes_read
            );
            throw ErrorCode_Failure;
        }
        m_decompressor.close();
        encoded_variable_t* converted_variable_ptr
                = reinterpret_cast<encoded_variable_t*>(m_read_buffer_ptr);
        for (size_t row_ix = 0; row_ix < m_num_row; row_ix++) {
            encoded_variable_t encoded_var = converted_variable_ptr[row_ix];
            m_column_based_variables[column_ix * m_num_row + row_ix] = encoded_var;
        }
    }
}

void LogtypeTable::open(char const* buffer, LogtypeMetadata const& metadata) {
    if (m_is_open) {
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_is_open = true;
    m_file_offset = buffer;
    m_current_row = 0;
    m_metadata = metadata;
    m_num_row = m_metadata.num_rows;
    m_num_columns = m_metadata.num_columns;
    m_buffer_size = m_num_row * sizeof(encoded_variable_t);
    m_read_buffer = std::make_unique<char[]>(m_buffer_size);
    m_read_buffer_ptr = m_read_buffer.get();
    m_ts_loaded = false;
    m_column_loaded.resize(m_num_columns, false);
    m_column_based_variables.resize(m_num_row * m_num_columns);
}

void LogtypeTable::close() {
    if (!m_is_open) {
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_column_loaded.clear();
    m_is_open = false;
    m_read_buffer_ptr = nullptr;
}

bool LogtypeTable::get_next_message(Message& msg) {
    if (!m_is_open) {
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    if (m_current_row == m_num_row) {
        return false;
    }
    size_t return_index = m_current_row;
    auto& writable_var_vector = msg.get_writable_vars();
    for (size_t column_index = 0; column_index < m_num_columns; column_index++) {
        writable_var_vector[column_index]
                = m_column_based_variables[column_index * m_num_row + return_index];
    }
    msg.set_timestamp(m_timestamps[return_index]);
    msg.set_file_id(m_file_ids[return_index]);
    m_current_row++;
    return true;
}

void LogtypeTable::get_next_row(
        std::vector<encoded_variable_t>& vars,
        size_t var_ix_begin,
        size_t var_ix_end
) const {
    for (size_t ix = var_ix_begin; ix < var_ix_end; ix++) {
        vars[ix] = m_column_based_variables[ix * m_num_row + m_current_row];
    }
}

void LogtypeTable::skip_row() {
    m_current_row++;
}

bool LogtypeTable::peek_next_ts(epochtime_t& ts) {
    if (m_current_row < m_num_row) {
        ts = m_timestamps[m_current_row];
        return true;
    }
    return false;
}

// loading the data in TS->file_id->variable columns should be the right order
void LogtypeTable::load_remaining_data_into_vec(
        std::vector<epochtime_t>& ts,
        std::vector<file_id_t>& id,
        std::vector<encoded_variable_t>& vars,
        std::vector<size_t> const& potential_matched_row
) {
    load_ts_into_vec(ts, potential_matched_row);
    load_file_id_into_vec(id, potential_matched_row);
    load_vars_into_vec(vars, potential_matched_row);
}

void LogtypeTable::load_timestamp() {
    m_timestamps.resize(m_num_row);
    size_t num_bytes_read = 0;
    char const* ts_start = m_file_offset + m_metadata.ts_offset;
    m_decompressor.open(ts_start, m_metadata.ts_size);
    m_decompressor.try_read(m_read_buffer_ptr, m_buffer_size, num_bytes_read);
    if (num_bytes_read != m_buffer_size) {
        SPDLOG_ERROR(
                "Wrong number of Bytes read: Expect: {}, Got: {}",
                m_buffer_size,
                num_bytes_read
        );
        throw ErrorCode_Failure;
    }
    m_decompressor.close();
    epochtime_t* converted_timestamp_ptr = reinterpret_cast<epochtime_t*>(m_read_buffer_ptr);
    for (size_t row_ix = 0; row_ix < m_num_row; row_ix++) {
        m_timestamps[row_ix] = converted_timestamp_ptr[row_ix];
    }
    m_ts_loaded = true;
}

void LogtypeTable::load_variable_columns(size_t var_ix_begin, size_t var_ix_end) {
    for (size_t var_ix = var_ix_begin; var_ix < var_ix_end; var_ix++) {
        if (m_column_loaded[var_ix] == false) {
            load_column(var_ix);
        }
    }
}

epochtime_t LogtypeTable::get_timestamp_at_offset(size_t offset) {
    if (!m_is_open) {
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    assert(offset < m_num_row);
    return m_timestamps[offset];
}

void LogtypeTable::get_message_at_offset(size_t offset, Message& msg) {
    if (!m_is_open) {
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    assert(offset < m_num_row);

    for (size_t column_index = 0; column_index < m_num_columns; column_index++) {
        msg.add_var(m_column_based_variables[column_index * m_num_row + offset]);
    }
}

// this aims to be a little bit more optimized
void LogtypeTable::load_column(size_t column_ix) {
    char const* var_start = m_file_offset + m_metadata.column_offset[column_ix];
    m_decompressor.open(var_start, m_metadata.column_size[column_ix]);
    size_t num_bytes_read;
    m_decompressor.try_read(m_read_buffer_ptr, m_buffer_size, num_bytes_read);
    if (num_bytes_read != m_buffer_size) {
        SPDLOG_ERROR(
                "Wrong number of Bytes read: Expect: {}, Got: {}",
                m_buffer_size,
                num_bytes_read
        );
        throw ErrorCode_Failure;
    }
    m_decompressor.close();
    encoded_variable_t* converted_variable_ptr
            = reinterpret_cast<encoded_variable_t*>(m_read_buffer_ptr);
    for (size_t row_ix = 0; row_ix < m_num_row; row_ix++) {
        encoded_variable_t encoded_var = converted_variable_ptr[row_ix];
        m_column_based_variables[column_ix * m_num_row + row_ix] = encoded_var;
    }
    m_column_loaded[column_ix] = true;
}

void LogtypeTable::load_file_id_into_vec(
        std::vector<file_id_t>& id,
        std::vector<size_t> const& potential_matched_row
) {
    size_t num_bytes_read = 0;
    char const* file_id_start = m_file_offset + m_metadata.file_id_offset;
    size_t last_matching_row_ix = potential_matched_row.back();
    size_t size_to_read = (last_matching_row_ix + 1) * sizeof(file_id_t);
    m_decompressor.open(file_id_start, m_metadata.file_id_size);
    m_decompressor.try_read(m_read_buffer_ptr, size_to_read, num_bytes_read);
    if (num_bytes_read != size_to_read) {
        SPDLOG_ERROR(
                "Wrong number of Bytes read: Expect: {}, Got: {}",
                size_to_read,
                num_bytes_read
        );
        throw ErrorCode_Failure;
    }
    m_decompressor.close();
    file_id_t* converted_file_id_ptr = reinterpret_cast<file_id_t*>(m_read_buffer_ptr);
    for (size_t ix = 0; ix < potential_matched_row.size(); ix++) {
        id[ix] = converted_file_id_ptr[potential_matched_row[ix]];
    }
}

void LogtypeTable::load_ts_into_vec(
        std::vector<epochtime_t>& ts,
        std::vector<size_t> const& potential_matched_row
) {
    if (!m_ts_loaded) {
        size_t num_bytes_read = 0;
        char const* ts_start = m_file_offset + m_metadata.ts_offset;
        size_t last_matching_row_ix = potential_matched_row.back();
        size_t size_to_read = (last_matching_row_ix + 1) * sizeof(epochtime_t);
        m_decompressor.open(ts_start, m_metadata.ts_size);
        m_decompressor.try_read(m_read_buffer_ptr, size_to_read, num_bytes_read);
        if (num_bytes_read != size_to_read) {
            SPDLOG_ERROR(
                    "Wrong number of Bytes read: Expect: {}, Got: {}",
                    size_to_read,
                    num_bytes_read
            );
            throw ErrorCode_Failure;
        }
        m_decompressor.close();
        epochtime_t* converted_timestamp_ptr = reinterpret_cast<epochtime_t*>(m_read_buffer_ptr);
        for (size_t ix = 0; ix < potential_matched_row.size(); ix++) {
            ts[ix] = converted_timestamp_ptr[potential_matched_row[ix]];
        }
    } else {
        for (size_t ix = 0; ix < potential_matched_row.size(); ix++) {
            ts[ix] = m_timestamps[potential_matched_row[ix]];
        }
    }
}

void LogtypeTable::load_vars_into_vec(
        std::vector<encoded_variable_t>& vars,
        std::vector<size_t> const& potential_matched_row
) {
    size_t num_bytes_read = 0;
    size_t last_matching_row_ix = potential_matched_row.back();
    size_t size_to_read = (last_matching_row_ix + 1) * sizeof(size_t);
    for (size_t column_ix = 0; column_ix < m_num_columns; column_ix++) {
        if (m_column_loaded[column_ix] == false) {
            char const* var_start = m_file_offset + m_metadata.column_offset[column_ix];
            m_decompressor.open(var_start, m_metadata.column_size[column_ix]);
            m_decompressor.try_read(m_read_buffer_ptr, size_to_read, num_bytes_read);
            if (num_bytes_read != size_to_read) {
                SPDLOG_ERROR(
                        "Wrong number of Bytes read: Expect: {}, Got: {}",
                        size_to_read,
                        num_bytes_read
                );
                throw ErrorCode_Failure;
            }
            m_decompressor.close();
            encoded_variable_t* converted_vars_ptr
                    = reinterpret_cast<encoded_variable_t*>(m_read_buffer_ptr);
            for (size_t ix = 0; ix < potential_matched_row.size(); ix++) {
                vars[ix * m_num_columns + column_ix]
                        = converted_vars_ptr[potential_matched_row[ix]];
            }
        } else {
            for (size_t ix = 0; ix < potential_matched_row.size(); ix++) {
                vars[ix * m_num_columns + column_ix] = m_column_based_variables
                        [column_ix * m_num_row + potential_matched_row[ix]];
            }
        }
    }
}
}  // namespace glt::streaming_archive::reader
