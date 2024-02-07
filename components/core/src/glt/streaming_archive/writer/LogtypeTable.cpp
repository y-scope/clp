#include "LogtypeTable.hpp"

namespace glt::streaming_archive::writer {
LogtypeTable::LogtypeTable(size_t num_columns) {
    m_num_columns = num_columns;
    m_variables.resize(num_columns);
    m_num_rows = 0;
}

void LogtypeTable::append_to_table(
        epochtime_t timestamp,
        file_id_t file_id,
        std::vector<encoded_variable_t> const& encoded_vars
) {
    if (encoded_vars.size() != m_num_columns) {
        SPDLOG_ERROR(
                "streaming_compression::writer::LogtypeTable: input doesn't match table dimension"
        );
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_num_rows++;
    for (size_t index = 0; index < m_num_columns; index++) {
        m_variables[index].push_back(encoded_vars[index]);
    }
    m_timestamp.push_back(timestamp);
    m_file_ids.push_back(file_id);
}
}  // namespace glt::streaming_archive::writer
