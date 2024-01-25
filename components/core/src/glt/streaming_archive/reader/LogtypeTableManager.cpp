#include "LogtypeTableManager.hpp"

// Boost libraries
#include <boost/filesystem.hpp>

namespace glt::streaming_archive::reader {
void LogtypeTableManager::open(std::string const& segment_path) {
    if (m_is_open) {
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_var_column_directory_path = segment_path + ".var";
    load_metadata();
    load_variables_segment();
    m_is_open = true;
}

void LogtypeTableManager::close() {
    // GLT TODO
    // if(!m_is_open) {
    //     throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    // }
    m_is_open = false;
    m_memory_mapped_segment_file.close();
    m_logtype_table_metadata.clear();
    m_var_column_directory_path.clear();
    m_logtype_table_order.clear();
    m_combined_table_order.clear();
}

void LogtypeTableManager::load_variables_segment() {
    std::string column_file = m_var_column_directory_path + '/' + cVarSegmentFileName;
    // Get the size of the compressed segment file
    boost::system::error_code boost_error_code;
    size_t column_file_size = boost::filesystem::file_size(column_file, boost_error_code);
    if (boost_error_code) {
        SPDLOG_ERROR(
                "streaming_archive::reader::Segment: Unable to obtain file size for segment: {}",
                column_file.c_str()
        );
        SPDLOG_ERROR("streaming_archive::reader::Segment: {}", boost_error_code.message().c_str());
        throw ErrorCode_Failure;
    }

    // Create read only memory mapped file
    boost::iostreams::mapped_file_params memory_map_params;
    memory_map_params.path = column_file;
    memory_map_params.flags = boost::iostreams::mapped_file::readonly;
    memory_map_params.length = column_file_size;
    memory_map_params.hint = m_memory_mapped_segment_file.data(
    );  // try to map it to the same memory location as previous memory mapped file
    m_memory_mapped_segment_file.open(memory_map_params);
    if (!m_memory_mapped_segment_file.is_open()) {
        SPDLOG_ERROR(
                "streaming_archive::reader:Segment: Unable to memory map the compressed segment "
                "with path: {}",
                column_file.c_str()
        );
        throw ErrorCode_Failure;
    }
}

void LogtypeTableManager::load_metadata() {
    m_logtype_table_metadata.clear();
    m_logtype_table_order.clear();
    m_combined_tables_metadata.clear();
    m_combined_table_info.clear();
    m_combined_table_order.clear();
    std::string metadata_path = m_var_column_directory_path + '/' + cVarMetadataFileName;

    // Get the size of the compressed segment file
    boost::system::error_code boost_error_code;
    size_t metadata_file_size = boost::filesystem::file_size(metadata_path, boost_error_code);
    if (boost_error_code) {
        SPDLOG_ERROR(
                "streaming_archive::reader::Segment: Unable to obtain file size for segment: {}",
                metadata_path.c_str()
        );
        SPDLOG_ERROR("streaming_archive::reader::Segment: {}", boost_error_code.message().c_str());
        throw ErrorCode_Failure;
    }

    // Create read only memory mapped file
    boost::iostreams::mapped_file_source memory_mapped_segment_file;
    boost::iostreams::mapped_file_params memory_map_params;
    memory_map_params.path = metadata_path;
    memory_map_params.flags = boost::iostreams::mapped_file::readonly;
    memory_map_params.length = metadata_file_size;
    memory_map_params.hint = memory_mapped_segment_file.data(
    );  // try to map it to the same memory location as previous memory mapped file
    memory_mapped_segment_file.open(memory_map_params);
    if (!memory_mapped_segment_file.is_open()) {
        SPDLOG_ERROR(
                "streaming_archive::reader:Segment: Unable to memory map the compressed segment "
                "with path: {}",
                metadata_path.c_str()
        );
        throw ErrorCode_Failure;
    }
#if USE_PASSTHROUGH_COMPRESSION
    streaming_compression::passthrough::Decompressor metadata_decompressor;
#elif USE_ZSTD_COMPRESSION
    streaming_compression::zstd::Decompressor metadata_decompressor;
#else
    static_assert(false, "Unsupported compression mode.");
#endif
    metadata_decompressor.open(memory_mapped_segment_file.data(), metadata_file_size);

    size_t logtype_count;
    LogtypeMetadata metadata_obj;
    CombinedMetadata combined_table_obj;
    size_t logtype_id;
    size_t compression_type;

    // read logtype metadata
    metadata_decompressor.exact_read((char*)&logtype_count, sizeof(size_t));
    for (size_t log_ix = 0; log_ix < logtype_count; log_ix++) {
        metadata_decompressor.exact_read((char*)&compression_type, sizeof(size_t));
        // handle variable tables that occupied the complete compressed stream
        if (compression_type == streaming_archive::LogtypeTableType::NonCombined) {
            metadata_decompressor.exact_read((char*)&logtype_id, sizeof(logtype_dictionary_id_t));
            metadata_obj.column_offset.clear();
            metadata_obj.column_size.clear();

            // row and columns
            metadata_decompressor.exact_read((char*)&metadata_obj.num_rows, sizeof(size_t));
            metadata_decompressor.exact_read((char*)&metadata_obj.num_columns, sizeof(size_t));

            size_t ts_begin, file_id_begin, first_var_col_begin;
            metadata_decompressor.exact_read((char*)&ts_begin, sizeof(size_t));
            metadata_decompressor.exact_read((char*)&file_id_begin, sizeof(size_t));
            metadata_decompressor.exact_read((char*)&first_var_col_begin, sizeof(size_t));

            metadata_obj.ts_offset = ts_begin;
            metadata_obj.ts_size = file_id_begin - ts_begin;
            metadata_obj.file_id_offset = file_id_begin;
            metadata_obj.file_id_size = first_var_col_begin - file_id_begin;

            size_t cur = first_var_col_begin;
            size_t next;
            for (size_t i = 0; i < metadata_obj.num_columns; i++) {
                metadata_obj.column_offset.push_back(cur);
                metadata_decompressor.exact_read((char*)&next, sizeof(size_t));
                if (next < cur) {
                    SPDLOG_ERROR("Corrupted metadata");
                    throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
                }
                size_t cur_column_size = next - cur;
                metadata_obj.column_size.push_back(cur_column_size);
                cur = next;
            }
            m_logtype_table_metadata[logtype_id] = metadata_obj;
            m_logtype_table_order.push_back(logtype_id);
        } else if (compression_type == streaming_archive::LogtypeTableType::Combined) {
            metadata_decompressor.exact_read((char*)&logtype_id, sizeof(logtype_dictionary_id_t));
            // combined table id
            size_t combined_table_ix;
            metadata_decompressor.exact_read(
                    (char*)&combined_table_ix,
                    sizeof(combined_table_id_t)
            );
            // row and columns
            metadata_decompressor.exact_read((char*)&combined_table_obj.num_rows, sizeof(size_t));
            metadata_decompressor.exact_read(
                    (char*)&combined_table_obj.num_columns,
                    sizeof(size_t)
            );
            // beginning offset
            size_t begin_offset;
            metadata_decompressor.exact_read((char*)&begin_offset, sizeof(size_t));
            combined_table_obj.combined_table_id = combined_table_ix;
            combined_table_obj.offset = begin_offset;

            m_combined_tables_metadata[logtype_id] = combined_table_obj;
            m_combined_table_order[combined_table_ix].push_back(logtype_id);
        } else {
            SPDLOG_ERROR("Unsupported metadata compression type {}", compression_type);
            throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
        }
    }

    // read logtype metadata.
    CombinedTableInfo table_info;
    metadata_decompressor.exact_read((char*)&m_combined_table_count, sizeof(size_t));
    for (combined_table_id_t table_ix = 0; table_ix < m_combined_table_count; table_ix++) {
        metadata_decompressor.exact_read((char*)&table_info.m_begin_offset, sizeof(size_t));
        metadata_decompressor.exact_read((char*)&table_info.m_size, sizeof(size_t));
        m_combined_table_info[table_ix] = table_info;
    }

    metadata_decompressor.close();
    memory_mapped_segment_file.close();
}
}  // namespace glt::streaming_archive::reader
