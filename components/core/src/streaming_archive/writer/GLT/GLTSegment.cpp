#include "GLTSegment.hpp"
#include "../../LogtypeSizeTracker.hpp"
#include <iostream>

using streaming_archive::glt::LogtypeSizeTracker;

namespace streaming_archive::writer {
    GLTSegment::~GLTSegment () {
        if (!m_segment_path.empty()) {
            SPDLOG_ERROR(
                "streaming_archive::writer::GLTSegment: GLTSegment {} not closed before being destroyed causing possible data loss",
                 m_segment_path.c_str()
            );
        }
    }

    void GLTSegment::open (const std::string& segments_dir_path, segment_id_t id,
                           int compression_level, double threshold) {
        if (!m_segment_path.empty()) {
            throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
        }

        m_id = id;

        // Construct segment path
        m_segment_path = segments_dir_path;
        m_segment_path += std::to_string(m_id);
        m_table_threshold = threshold;
        m_compression_level = compression_level;
        m_compressed_size = 0;
        m_uncompressed_size = 0;
    }

    void GLTSegment::close () {
        compress_logtype_tables_to_disk();
        m_segment_path.clear();
    }

    bool GLTSegment::is_open () const {
        return !m_segment_path.empty();
    }

    void GLTSegment::compress_logtype_tables_to_disk () {

        std::string segment_var_directory = m_segment_path + cVariablesFileExtension;
        size_t total_size = 0;
        // Create output directory in case it doesn't exist
        auto error_code = create_directory(segment_var_directory, 0700, true);
        if (ErrorCode_Success != error_code) {
            SPDLOG_ERROR("Failed to create {} - {}", segment_var_directory, strerror(errno));
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }

        std::string var_column_file = segment_var_directory + "/" + cVarSegmentFileName;
        m_logtype_table_writer.open(var_column_file, FileWriter::OpenMode::CREATE_FOR_WRITING);

        std::set<LogtypeSizeTracker, std::greater<LogtypeSizeTracker>> ordered_logtype_tables;
        for (const auto& iter : m_logtype_variables) {
            logtype_dictionary_id_t logtype_id = iter.first;
            const auto& logtype_table = iter.second;
            size_t logtype_size = LogtypeSizeTracker::get_table_size(logtype_table.get_num_columns(), logtype_table.get_num_rows());
            ordered_logtype_tables.emplace(logtype_id, logtype_size);
            total_size += logtype_size;
        }

        // Metadata format
        // type, (1-byte) [0-1] -> single logtype table, combined logtype
        // num_logtype_ids (this can be known at the very beginning)
        // if type 0-> logtype_id, num_column, num_row, offset, file_id_offset, first_column_offset, second_column_offset... last_column_offset, end_offset
        // if type 1-> logtype_id, num_column, num_row, offset
        // number of combined_table
        // [table_id(64bit), offset, size]+
        std::string metadata_file = segment_var_directory + "/" + cVarMetadataFileName;
        m_metadata_writer.open(metadata_file, FileWriter::OpenMode::CREATE_FOR_WRITING);
        open_metadata_compressor();

        // First, write the numbers of all logtypes
        size_t logtype_count = m_logtype_variables.size();
        m_metadata_compressor.write(reinterpret_cast<const char*>(&logtype_count),
                                    sizeof(size_t));

        size_t accumulated_size = 0;
        double threshold = m_table_threshold / 100;
        combined_table_id_t combined_table_id = 0;
        std::vector<logtype_dictionary_id_t> accumulated_logtype;
        // hold the conbined_table_id -> info mapping
        std::map<size_t, CombinedTableInfo> combined_tables_info;

        for(const auto& logtype : ordered_logtype_tables) {
            logtype_dictionary_id_t logtype_id = logtype.get_id();
            size_t table_size = logtype.get_size();
            // if the logtype is large enough
            if (double(table_size) / total_size > threshold) {
                size_t compression_type = streaming_archive::GLT::LogtypeTableType::NonCombined;
                m_metadata_compressor.write(reinterpret_cast<const char*>(&compression_type),
                                            sizeof(size_t));
                const auto& variable_segment_writer = m_logtype_variables.at(logtype_id);
                write_single_logtype(logtype_id, variable_segment_writer);
            } else {
                // if the logtype is small, we accumulate everything.
                accumulated_size += table_size;
                accumulated_logtype.push_back(logtype_id);
                if ((double(accumulated_size) / total_size) > threshold) {
                    write_accumulated_logtype(accumulated_logtype, combined_table_id,
                                             combined_tables_info);
                    accumulated_size = 0;
                    accumulated_logtype.clear();
                }
            }
        }
        // write remaining logtype
        write_accumulated_logtype(accumulated_logtype, combined_table_id, combined_tables_info);

        // store info of combined_tables
        // number of combined tables
        size_t combined_table_id_count = combined_table_id;
        m_metadata_compressor.write(reinterpret_cast<const char*>(&combined_table_id_count),
                                    sizeof(size_t));
        size_t sanity_check_item = 0;
        for (const auto& iter : combined_tables_info) {
            if (iter.first != sanity_check_item) {
                SPDLOG_ERROR("expecting combined table id {} but got {}", sanity_check_item,
                             iter.first);
            }
            sanity_check_item++;
            m_metadata_compressor.write(
                    reinterpret_cast<const char*>(&iter.second.m_begin_offset),
                    sizeof(size_t));
            m_metadata_compressor.write(reinterpret_cast<const char*>(&iter.second.m_size),
                                        sizeof(size_t));
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

    void GLTSegment::write_accumulated_logtype (
            const std::vector<logtype_dictionary_id_t>& accumulated_logtype,
            combined_table_id_t& combined_table_id,
            std::map<size_t, CombinedTableInfo>& combined_tables_info) {
        if (accumulated_logtype.size() == 0) {
            return;
        }
        open_combined_table_compressor();
        size_t compression_type = streaming_archive::GLT::LogtypeTableType::Combined;
        // for each logtype accumulated
        // if type 1-> num_column, num_row, offset(uncompressed offset) (because offset can be inferred from this)
        // also need to know the stream id to know what's the compression size.
        size_t combined_table_beginning_offset = m_logtype_table_writer.get_pos();
        for (const auto& logtype_id : accumulated_logtype) {

            const auto& logtype_table = m_logtype_variables.at(logtype_id);

            // write the compression type
            m_metadata_compressor.write(reinterpret_cast<const char*>(&compression_type),
                                        sizeof(size_t));
            // write the logtype id
            m_metadata_compressor.write(reinterpret_cast<const char*>(&logtype_id),
                                        sizeof(size_t));
            // write the stream id
            m_metadata_compressor.write(reinterpret_cast<const char*>(&combined_table_id),
                                        sizeof(combined_table_id_t));

            // write the number of rows and columns
            size_t num_row = logtype_table.get_num_rows();
            size_t num_column = logtype_table.get_num_columns();
            m_metadata_compressor.write(reinterpret_cast<const char*>(&num_row),
                                        sizeof(size_t));
            m_metadata_compressor.write(reinterpret_cast<const char*>(&num_column),
                                        sizeof(size_t));

            // write the offset(uncompressed)
            size_t logtype_beginning_offset = m_combined_compressor.get_pos();
            m_metadata_compressor.write(
                    reinterpret_cast<const char*>(&logtype_beginning_offset), sizeof(size_t));

            const auto& timestamps_data = logtype_table.get_timestamps();
            const uint64_t timestamp_size = timestamps_data.size() * sizeof(epochtime_t);
            m_combined_compressor.write(reinterpret_cast<const char*>(timestamps_data.data()),
                               timestamp_size);

            const auto& file_ids = logtype_table.get_file_ids();
            const uint64_t file_id_size = file_ids.size() * sizeof(file_id_t);
            m_combined_compressor.write(reinterpret_cast<const char*>(file_ids.data()), file_id_size);

            const auto& columns = logtype_table.get_variables();
            for (size_t column_ix = 0; column_ix < columns.size(); column_ix++) {
                const auto& column_data = columns[column_ix];
                const uint64_t column_data_size =
                        column_data.size() * sizeof(encoded_variable_t);
                m_combined_compressor.write(reinterpret_cast<const char*>(column_data.data()),
                                   column_data_size);
            }
        }
        m_combined_compressor.close();
        size_t table_size = m_logtype_table_writer.get_pos() - combined_table_beginning_offset;
        combined_tables_info.emplace(std::piecewise_construct,
                                     std::forward_as_tuple(combined_table_id),
                                     std::forward_as_tuple(combined_table_beginning_offset,
                                                           table_size));
        combined_table_id++;
    }

    // compress single logtype table to disk. every column will be compressed as individual stream.
    void GLTSegment::write_single_logtype (logtype_dictionary_id_t logtype_id,
                                           const LogtypeTable& m_logtype_variables) {
        // metadata format->
        // logtype_id, num_column, num_row, ts_offset, file_id_offset, first_column_offset, second_column_offset... last_column_offset, end_offset
        m_metadata_compressor.write(reinterpret_cast<const char*>(&logtype_id),
                                    sizeof(logtype_dictionary_id_t));

        // Write number of rows.
        size_t num_row = m_logtype_variables.get_num_rows();
        size_t num_column = m_logtype_variables.get_num_columns();
        m_metadata_compressor.write(reinterpret_cast<const char*>(&num_row), sizeof(size_t));
        m_metadata_compressor.write(reinterpret_cast<const char*>(&num_column),
                                    sizeof(size_t));

        // write beginning offset
        size_t current_pos = m_logtype_table_writer.get_pos();
        m_metadata_compressor.write(reinterpret_cast<const char*>(&current_pos),
                                    sizeof(size_t));

        // Write timestamps
        open_single_table_compressor();
        const auto& timestamps_data = m_logtype_variables.get_timestamps();
        const uint64_t timestamp_size = timestamps_data.size() * sizeof(epochtime_t);
        m_single_compressor.write(reinterpret_cast<const char*>(timestamps_data.data()),
                                   timestamp_size);
        m_single_compressor.close();


        // write beginning offset
        current_pos = m_logtype_table_writer.get_pos();
        m_metadata_compressor.write(reinterpret_cast<const char*>(&current_pos),
                                    sizeof(size_t));

        // Write file_id
        open_single_table_compressor();
        // Write file_ids
        const auto& file_ids = m_logtype_variables.get_file_ids();
        const uint64_t file_id_size = file_ids.size() * sizeof(file_id_t);
        m_single_compressor.write(reinterpret_cast<const char*>(file_ids.data()),
                                   file_id_size);
        m_single_compressor.close();


        // Write columns one by one
        const auto& columns = m_logtype_variables.get_variables();
        for (size_t column_ix = 0; column_ix < columns.size(); column_ix++) {
            const auto& column_data = columns[column_ix];
            const uint64_t column_data_size = column_data.size() * sizeof(encoded_variable_t);

            // write beginning offset
            current_pos = m_logtype_table_writer.get_pos();
            m_metadata_compressor.write(reinterpret_cast<const char*>(&current_pos),
                                        sizeof(size_t));

            open_single_table_compressor();
            m_single_compressor.write(reinterpret_cast<const char*>(column_data.data()),
                                       column_data_size);
            m_single_compressor.close();
        }
        current_pos = m_logtype_table_writer.get_pos();
        m_metadata_compressor.write(reinterpret_cast<const char*>(&current_pos),
                                    sizeof(size_t));
    };

    void GLTSegment::open_single_table_compressor () {
#if USE_PASSTHROUGH_COMPRESSION
        m_single_compressor.open(m_file_writer);
#else
        m_single_compressor.open(m_logtype_table_writer, m_compression_level);
#endif
    }

    void GLTSegment::open_combined_table_compressor () {
#if USE_PASSTHROUGH_COMPRESSION
        m_combined_compressor.open(m_file_writer);
#else
        m_combined_compressor.open(m_logtype_table_writer, m_compression_level);
#endif
    }

    void GLTSegment::open_metadata_compressor () {
#if USE_PASSTHROUGH_COMPRESSION
        m_metadata_compressor.open(m_metadata_writer);
#else
        m_metadata_compressor.open(m_metadata_writer, m_compression_level);
#endif
    }

    void GLTSegment::increment_uncompressed_size (size_t file_size) {
        m_uncompressed_size += file_size;
    }

    // return the offset of the row
    size_t GLTSegment::append_var_to_segment (logtype_dictionary_id_t logtype_id,
                                              epochtime_t timestamp,
                                              file_id_t file_id,
                                              const std::vector<encoded_variable_t>& encoded_vars) {
        if (m_logtype_variables.find(logtype_id) == m_logtype_variables.end()) {
            m_logtype_variables.emplace(logtype_id, encoded_vars.size());
        }
        auto iter = m_logtype_variables.find(logtype_id);
        // Here, say if there are already 3 rows, then the next offset start at index 3
        size_t offset = iter->second.get_num_rows();
        iter->second.append_to_variable_segment(timestamp, file_id, encoded_vars);
        return offset;
    }

    uint64_t GLTSegment::get_uncompressed_size () {
        return m_uncompressed_size;
    }

    size_t GLTSegment::get_compressed_size () {
        if (!m_segment_path.empty()) {
            SPDLOG_ERROR(
                    "streaming_archive::writer::GLTSegment: get_compressed_size called before closing the segment");
            throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
        }
        return m_compressed_size;
    }
}