#include "GLTSegment.hpp"

#include <iostream>

#include "../LogtypeSizeTracker.hpp"

using glt::streaming_archive::LogtypeSizeTracker;

namespace glt::streaming_archive::writer {
GLTSegment::~GLTSegment() {
    if (!m_segment_path.empty()) {
        SPDLOG_ERROR(
                "streaming_archive::writer::GLTSegment: GLTSegment {} not closed before being "
                "destroyed causing possible data loss",
                m_segment_path.c_str()
        );
    }
}

void GLTSegment::open(
        std::string const& segments_dir_path,
        segment_id_t id,
        int compression_level,
        double threshold
) {
    if (!m_segment_path.empty()) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    m_id = id;
    m_uncompressed_size = 0;
    m_compressed_size = 0;

    // Construct segment path
    m_segment_path = segments_dir_path;
    m_segment_path += std::to_string(m_id);
    m_table_threshold = threshold;
    m_compression_level = compression_level;
}

void GLTSegment::close() {
    compress_logtype_tables_to_disk();
    m_segment_path.clear();
}

bool GLTSegment::is_open() const {
    return !m_segment_path.empty();
}

void GLTSegment::compress_logtype_tables_to_disk() {
    std::string segment_var_directory = m_segment_path + cVariablesFileExtension;
    // Create output directory in case it doesn't exist
    auto error_code = create_directory(segment_var_directory, 0700, true);
    if (ErrorCode_Success != error_code) {
        SPDLOG_ERROR("Failed to create {} - {}", segment_var_directory, strerror(errno));
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }

    std::string var_column_file = segment_var_directory + "/" + cVarSegmentFileName;
    m_logtype_table_writer.open(var_column_file, FileWriter::OpenMode::CREATE_FOR_WRITING);

    // Sort logtype table based on size with set and get total size
    size_t total_size = 0;
    std::set<LogtypeSizeTracker, std::greater<LogtypeSizeTracker>> ordered_logtype_tables;
    for (auto const& iter : m_logtype_variables) {
        logtype_dictionary_id_t logtype_id = iter.first;
        auto const& logtype_table = iter.second;
        size_t logtype_size = LogtypeSizeTracker::get_table_size(
                logtype_table.get_num_columns(),
                logtype_table.get_num_rows()
        );
        ordered_logtype_tables.emplace(logtype_id, logtype_size);
        total_size += logtype_size;
    }

    /** Metadata format
     * [Number of logtype]
     * [logtype data]+
     *      [type = 0] -> logtype_id, num_column, num_row, offset, file_id_offset,
     * first_column_offset, second_column_offset... last_column_offset, end_offset [type = 1] ->
     * logtype_id, num_column, num_row, offset [number of combined_table] [table_id(64bit), offset,
     * size]+
     */
    std::string metadata_file = segment_var_directory + "/" + cVarMetadataFileName;
    m_metadata_writer.open(metadata_file, FileWriter::OpenMode::CREATE_FOR_WRITING);
    open_metadata_compressor();

    // write the numbers of all logtypes
    size_t logtype_count = m_logtype_variables.size();
    m_metadata_compressor.write(reinterpret_cast<char const*>(&logtype_count), sizeof(size_t));

    size_t accumulated_size = 0;
    double threshold = m_table_threshold / 100;

    std::vector<logtype_dictionary_id_t> accumulated_logtype;
    std::map<combined_table_id_t, CombinedTableInfo> combined_tables_info;

    for (auto const& logtype : ordered_logtype_tables) {
        logtype_dictionary_id_t logtype_id = logtype.get_id();
        size_t table_size = logtype.get_size();
        // if the logtype is large enough, write is as a single table
        if (double(table_size) / total_size > threshold) {
            write_single_logtype(logtype_id);
        } else {
            // if the logtype is small, we accumulate everything.
            accumulated_size += table_size;
            accumulated_logtype.push_back(logtype_id);
            if ((double(accumulated_size) / total_size) > threshold) {
                write_combined_logtype(accumulated_logtype, combined_tables_info);
                accumulated_size = 0;
                accumulated_logtype.clear();
            }
        }
    }
    // Don't forget to write remaining logtype tables
    if (accumulated_size > 0) {
        write_combined_logtype(accumulated_logtype, combined_tables_info);
    }

    // store info of combined_tables
    size_t combined_table_id_count = combined_tables_info.size();
    m_metadata_compressor.write(
            reinterpret_cast<char const*>(&combined_table_id_count),
            sizeof(size_t)
    );

    for (auto const& iter : combined_tables_info) {
        m_metadata_compressor.write(
                reinterpret_cast<char const*>(&iter.second.m_begin_offset),
                sizeof(combined_table_id_t)
        );
        m_metadata_compressor.write(
                reinterpret_cast<char const*>(&iter.second.m_size),
                sizeof(size_t)
        );
    }

    m_logtype_table_writer.flush();
    size_t compressed_total_size = m_logtype_table_writer.get_pos();
    m_logtype_table_writer.close();

    // close metadata writer
    m_metadata_compressor.flush();
    m_metadata_compressor.close();
    m_metadata_writer.close();

    m_compressed_size = compressed_total_size;
    m_logtype_variables.clear();
}

void GLTSegment::write_combined_logtype(
        std::vector<logtype_dictionary_id_t> const& accumulated_logtype,
        std::map<combined_table_id_t, CombinedTableInfo>& combined_tables_info
) {
    open_combined_table_compressor();
    combined_table_id_t combined_table_id = combined_tables_info.size();
    size_t compression_type = streaming_archive::LogtypeTableType::Combined;
    size_t combined_table_beginning_offset = m_logtype_table_writer.get_pos();
    for (auto const& logtype_id : accumulated_logtype) {
        auto const& logtype_table = m_logtype_variables.at(logtype_id);

        // Metadata
        // each combined logtype has the following metadata
        // [type], [logtype_id], [combined_table_id], [num_column], [num_row], [uncompressed offset]

        // write the compression type
        m_metadata_compressor.write(
                reinterpret_cast<char const*>(&compression_type),
                sizeof(size_t)
        );
        // write the logtype id
        m_metadata_compressor.write(reinterpret_cast<char const*>(&logtype_id), sizeof(size_t));
        // write the combined table id
        m_metadata_compressor.write(
                reinterpret_cast<char const*>(&combined_table_id),
                sizeof(combined_table_id_t)
        );

        // write the number of rows and columns
        size_t num_row = logtype_table.get_num_rows();
        size_t num_column = logtype_table.get_num_columns();
        m_metadata_compressor.write(reinterpret_cast<char const*>(&num_row), sizeof(size_t));
        m_metadata_compressor.write(reinterpret_cast<char const*>(&num_column), sizeof(size_t));

        // write the offset(uncompressed)
        size_t logtype_beginning_offset = m_combined_compressor.get_pos();
        m_metadata_compressor.write(
                reinterpret_cast<char const*>(&logtype_beginning_offset),
                sizeof(size_t)
        );

        // Write actual data
        auto const& timestamps_data = logtype_table.get_timestamps();
        uint64_t const timestamp_size = timestamps_data.size() * sizeof(epochtime_t);
        m_combined_compressor.write(
                reinterpret_cast<char const*>(timestamps_data.data()),
                timestamp_size
        );

        auto const& file_ids = logtype_table.get_file_ids();
        uint64_t const file_id_size = file_ids.size() * sizeof(file_id_t);
        m_combined_compressor.write(reinterpret_cast<char const*>(file_ids.data()), file_id_size);

        auto const& columns = logtype_table.get_variables();
        for (size_t column_ix = 0; column_ix < columns.size(); column_ix++) {
            auto const& column_data = columns[column_ix];
            uint64_t const column_data_size = column_data.size() * sizeof(encoded_variable_t);
            m_combined_compressor.write(
                    reinterpret_cast<char const*>(column_data.data()),
                    column_data_size
            );
        }
    }
    m_combined_compressor.close();
    // update the compressed combined table size.
    size_t table_size = m_logtype_table_writer.get_pos() - combined_table_beginning_offset;
    combined_tables_info.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(combined_table_id),
            std::forward_as_tuple(combined_table_beginning_offset, table_size)
    );
}

void GLTSegment::write_single_logtype(logtype_dictionary_id_t logtype_id) {
    // Get logtype table based on ID
    auto const& logtype_table = m_logtype_variables.at(logtype_id);

    /** metadata format->
     * compression type, logtype_id, num_column, num_row, ts_offset, file_id_offset,
     * first_column_offset, second_column_offset... last_column_offset, end_offset
     */
    // compression type and logtype ID
    size_t compression_type = streaming_archive::LogtypeTableType::NonCombined;
    m_metadata_compressor.write(reinterpret_cast<char const*>(&compression_type), sizeof(size_t));
    m_metadata_compressor.write(
            reinterpret_cast<char const*>(&logtype_id),
            sizeof(logtype_dictionary_id_t)
    );

    // Write number of rows.
    size_t num_row = logtype_table.get_num_rows();
    size_t num_column = logtype_table.get_num_columns();
    m_metadata_compressor.write(reinterpret_cast<char const*>(&num_row), sizeof(size_t));
    m_metadata_compressor.write(reinterpret_cast<char const*>(&num_column), sizeof(size_t));

    // write ts_offset
    size_t current_pos = m_logtype_table_writer.get_pos();
    m_metadata_compressor.write(reinterpret_cast<char const*>(&current_pos), sizeof(size_t));

    // Write timestamps
    open_single_table_compressor();
    auto const& timestamps_data = logtype_table.get_timestamps();
    uint64_t const timestamp_size = timestamps_data.size() * sizeof(epochtime_t);
    m_single_compressor.write(
            reinterpret_cast<char const*>(timestamps_data.data()),
            timestamp_size
    );
    m_single_compressor.close();

    // write file_id_offset
    current_pos = m_logtype_table_writer.get_pos();
    m_metadata_compressor.write(reinterpret_cast<char const*>(&current_pos), sizeof(size_t));

    // Write file_id
    open_single_table_compressor();
    auto const& file_ids = logtype_table.get_file_ids();
    uint64_t const file_id_size = file_ids.size() * sizeof(file_id_t);
    m_single_compressor.write(reinterpret_cast<char const*>(file_ids.data()), file_id_size);
    m_single_compressor.close();

    // Write columns one by one
    auto const& columns = logtype_table.get_variables();
    for (size_t column_ix = 0; column_ix < columns.size(); column_ix++) {
        auto const& column_data = columns[column_ix];
        uint64_t const column_data_size = column_data.size() * sizeof(encoded_variable_t);

        // write column_offset offset
        current_pos = m_logtype_table_writer.get_pos();
        m_metadata_compressor.write(reinterpret_cast<char const*>(&current_pos), sizeof(size_t));

        // write variable column data
        open_single_table_compressor();
        m_single_compressor.write(
                reinterpret_cast<char const*>(column_data.data()),
                column_data_size
        );
        m_single_compressor.close();
    }
    // write end offset
    current_pos = m_logtype_table_writer.get_pos();
    m_metadata_compressor.write(reinterpret_cast<char const*>(&current_pos), sizeof(size_t));
}

void GLTSegment::open_single_table_compressor() {
#if USE_PASSTHROUGH_COMPRESSION
    m_single_compressor.open(m_file_writer);
#else
    m_single_compressor.open(m_logtype_table_writer, m_compression_level);
#endif
}

void GLTSegment::open_combined_table_compressor() {
#if USE_PASSTHROUGH_COMPRESSION
    m_combined_compressor.open(m_file_writer);
#else
    m_combined_compressor.open(m_logtype_table_writer, m_compression_level);
#endif
}

void GLTSegment::open_metadata_compressor() {
#if USE_PASSTHROUGH_COMPRESSION
    m_metadata_compressor.open(m_metadata_writer);
#else
    m_metadata_compressor.open(m_metadata_writer, m_compression_level);
#endif
}

// return the offset of the row
size_t GLTSegment::append_to_segment(
        logtype_dictionary_id_t logtype_id,
        epochtime_t timestamp,
        file_id_t file_id,
        std::vector<encoded_variable_t> const& encoded_vars
) {
    if (m_logtype_variables.find(logtype_id) == m_logtype_variables.end()) {
        m_logtype_variables.emplace(logtype_id, encoded_vars.size());
    }
    auto iter = m_logtype_variables.find(logtype_id);
    // Offset start from 0. so current_offsert = num_rows - 1
    // and the offset after insertion is num_rows
    size_t offset = iter->second.get_num_rows();
    iter->second.append_to_table(timestamp, file_id, encoded_vars);

    m_uncompressed_size += sizeof(epochtime_t) + sizeof(file_id_t)
                           + sizeof(encoded_variable_t) * encoded_vars.size();
    return offset;
}

uint64_t GLTSegment::get_uncompressed_size() {
    return m_uncompressed_size;
}

size_t GLTSegment::get_compressed_size() {
    if (!m_segment_path.empty()) {
        SPDLOG_ERROR("streaming_archive::writer::GLTSegment: get_compressed_size called before "
                     "closing the segment");
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }
    return m_compressed_size;
}
}  // namespace glt::streaming_archive::writer
