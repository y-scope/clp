#include "CombinedLogtypeTable.hpp"

namespace glt::streaming_archive::reader {

CombinedLogtypeTable::CombinedLogtypeTable() {
    // try to reuse a buffer to avoid malloc & free
    m_buffer_size = 0;
    m_is_logtype_open = false;
    m_is_open = false;
}

void CombinedLogtypeTable::open(combined_table_id_t table_id) {
    assert(m_is_open == false);
    m_table_id = table_id;
    m_is_open = true;
}

void CombinedLogtypeTable::open_and_read_once_only(
        logtype_dictionary_id_t logtype_id,
        combined_table_id_t combined_table_id,
        streaming_compression::Decompressor& decompressor,
        std::unordered_map<logtype_dictionary_id_t, CombinedMetadata> const& metadata
) {
    assert(m_is_open == false);
    assert(m_is_logtype_open == false);

    m_table_id = combined_table_id;
    m_logtype_id = logtype_id;

    // add decompressor to the correct offset
    auto const& logtype_metadata = metadata.at(logtype_id);
    size_t table_offset = logtype_metadata.offset;
    decompressor.seek_from_begin(table_offset);

    // variable initialization
    m_current_row = 0;
    m_num_row = logtype_metadata.num_rows;
    m_num_columns = logtype_metadata.num_columns;

    // handle buffer. resize buffer if it's too small
    // max required buffer size should be data from one column
    size_t required_buffer_size = m_num_row * sizeof(uint64_t);
    std::unique_ptr<char[]> read_buffer = std::make_unique<char[]>(required_buffer_size);
    load_logtype_table_data(decompressor, read_buffer.get());
    m_is_logtype_open = true;
    m_is_open = true;
}

void CombinedLogtypeTable::load_logtype_table(
        logtype_dictionary_id_t logtype_id,
        streaming_compression::Decompressor& decompressor,
        std::unordered_map<logtype_dictionary_id_t, CombinedMetadata> const& metadata
) {
    assert(m_is_open);
    assert(m_is_logtype_open == false);

    m_logtype_id = logtype_id;

    // seek decompressor to the correct offset
    auto const& logtype_metadata = metadata.at(logtype_id);
    size_t table_offset = logtype_metadata.offset;
    decompressor.seek_from_begin(table_offset);

    // variable initialization
    m_current_row = 0;
    m_num_row = logtype_metadata.num_rows;
    m_num_columns = logtype_metadata.num_columns;

    // handle buffer. resize buffer if it's too small
    // max required buffer size is data from one column
    size_t required_buffer_size = m_num_row * sizeof(uint64_t);
    if (m_buffer_size < required_buffer_size) {
        m_buffer_size = required_buffer_size;
        m_read_buffer = std::make_unique<char[]>(required_buffer_size);
    }

    load_logtype_table_data(decompressor, m_read_buffer.get());

    m_is_logtype_open = true;
}

void CombinedLogtypeTable::close_logtype_table() {
    assert(m_is_logtype_open);
    m_timestamps.clear();
    m_file_ids.clear();
    m_column_based_variables.clear();
    m_is_logtype_open = false;
}

void CombinedLogtypeTable::close() {
    assert(m_is_open == true);
    // GLT TODO
    // assert(m_is_logtype_open == true);
    m_is_open = false;
}

bool CombinedLogtypeTable::get_next_message(Message& msg) {
    assert(m_is_open);
    assert(m_is_logtype_open);
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

bool CombinedLogtypeTable::get_next_message_partial(Message& msg, size_t l, size_t r) {
    if (m_current_row == m_num_row) {
        return false;
    }
    for (size_t ix = l; ix < r; ix++) {
        msg.get_writable_vars()[ix] = m_column_based_variables[ix * m_num_row + m_current_row];
    }
    msg.set_timestamp(m_timestamps[m_current_row]);
    msg.set_file_id(m_file_ids[m_current_row]);
    return true;
}

void CombinedLogtypeTable::get_remaining_message(Message& msg, size_t l, size_t r) {
    for (size_t ix = 0; ix < l; ix++) {
        msg.get_writable_vars()[ix] = m_column_based_variables[ix * m_num_row + m_current_row];
    }
    for (size_t ix = r; ix < m_num_columns; ix++) {
        msg.get_writable_vars()[ix] = m_column_based_variables[ix * m_num_row + m_current_row];
    }
    m_current_row++;
}

void CombinedLogtypeTable::skip_next_row() {
    m_current_row++;
}

epochtime_t CombinedLogtypeTable::get_timestamp_at_offset(size_t offset) {
    if (!m_is_open) {
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    assert(offset < m_num_row);
    return m_timestamps[offset];
}

void CombinedLogtypeTable::get_message_at_offset(size_t offset, Message& msg) {
    if (!m_is_open) {
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    assert(offset < m_num_row);

    for (size_t column_index = 0; column_index < m_num_columns; column_index++) {
        msg.add_var(m_column_based_variables[column_index * m_num_row + offset]);
    }
}

void CombinedLogtypeTable::load_logtype_table_data(
        streaming_compression::Decompressor& decompressor,
        char* read_buffer
) {
    // now we can start to read the variables. first figure out how many rows are there
    size_t num_bytes_read = 0;
    // read out the time stamp
    size_t ts_size = m_num_row * sizeof(epochtime_t);
    m_timestamps.resize(m_num_row);
    decompressor.try_read(read_buffer, ts_size, num_bytes_read);
    if (num_bytes_read != ts_size) {
        SPDLOG_ERROR("Wrong number of Bytes read: Expect: {}, Got: {}", ts_size, num_bytes_read);
        throw ErrorCode_Failure;
    }
    epochtime_t* converted_timestamp_ptr = reinterpret_cast<epochtime_t*>(read_buffer);
    for (size_t row_ix = 0; row_ix < m_num_row; row_ix++) {
        m_timestamps[row_ix] = converted_timestamp_ptr[row_ix];
    }

    m_file_ids.resize(m_num_row);
    size_t file_id_size = sizeof(file_id_t) * m_num_row;
    decompressor.try_read(read_buffer, file_id_size, num_bytes_read);
    if (num_bytes_read != file_id_size) {
        SPDLOG_ERROR(
                "Wrong number of Bytes read: Expect: {}, Got: {}",
                m_buffer_size,
                num_bytes_read
        );
        throw ErrorCode_Failure;
    }
    file_id_t* converted_file_id_ptr = reinterpret_cast<file_id_t*>(read_buffer);
    for (size_t row_ix = 0; row_ix < m_num_row; row_ix++) {
        m_file_ids[row_ix] = converted_file_id_ptr[row_ix];
    }

    m_column_based_variables.resize(m_num_row * m_num_columns);
    for (int column_ix = 0; column_ix < m_num_columns; column_ix++) {
        size_t column_size = sizeof(encoded_variable_t) * m_num_row;
        decompressor.try_read(read_buffer, column_size, num_bytes_read);
        if (num_bytes_read != column_size) {
            SPDLOG_ERROR(
                    "Wrong number of Bytes read: Expect: {}, Got: {}",
                    column_size,
                    num_bytes_read
            );
            throw ErrorCode_Failure;
        }
        encoded_variable_t* converted_variable_ptr
                = reinterpret_cast<encoded_variable_t*>(read_buffer);
        for (size_t row_ix = 0; row_ix < m_num_row; row_ix++) {
            encoded_variable_t encoded_var = converted_variable_ptr[row_ix];
            m_column_based_variables[column_ix * m_num_row + row_ix] = encoded_var;
        }
    }
}
}  // namespace glt::streaming_archive::reader
