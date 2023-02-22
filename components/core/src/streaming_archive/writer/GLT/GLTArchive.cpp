#include "GLTArchive.hpp"

// C++ libraries
#include <iostream>
#include <string>
#include <vector>

// Project headers
#include "../../../compressor_frontend/Constants.hpp"
#include "../../../EncodedVariableInterpreter.hpp"
#include "../../../clp/utils.hpp"
#include "../../GLTMetadataDB.hpp"

namespace streaming_archive::writer {
    GLTArchive::GLTArchive () {
        m_metadata_db = std::make_unique<GLTMetadataDB>();
    }

    void GLTArchive::open_derived (const UserConfig& user_config) {
        m_combine_threshold = user_config.glt_combine_threshold;
        // Save file_id to file name mapping to disk
        std::string file_id_file_path = m_path + '/' + cFileNameDictFilename;
        try {
            m_filename_dict_writer.open(file_id_file_path,
                                        FileWriter::OpenMode::CREATE_IF_NONEXISTENT_FOR_SEEKABLE_WRITING);
        } catch (FileWriter::OperationFailed& e) {
            SPDLOG_CRITICAL("Failed to create file: {}", file_id_file_path.c_str());
            throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
        }
    }

    void GLTArchive::close_derived () {
        // Close segments if necessary
        if (m_message_order_table.is_open()) {
            close_segment_and_persist_file_metadata(m_message_order_table,
                                                    m_glt_segment,
                                                    m_files_in_segment,
                                                    m_logtype_ids_in_segment,
                                                    m_var_ids_in_segment);
            m_logtype_ids_in_segment.clear();
            m_var_ids_in_segment.clear();
        }
        m_filename_dict_writer.flush();
        m_filename_dict_writer.close();
    }

    void GLTArchive::create_and_open_file (const std::string& path, group_id_t group_id,
                                           const boost::uuids::uuid& orig_file_id,
                                           size_t split_ix) {
        if (m_file != nullptr) {
            throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
        }
        m_glt_file = new GLTFile(m_uuid_generator(), orig_file_id, path, group_id, split_ix);
        m_file = m_glt_file;
        m_file->open();
        std::string file_name_to_write = path + '\n';
        m_filename_dict_writer.write(file_name_to_write.c_str(), file_name_to_write.size());
    }

    void GLTArchive::write_msg (epochtime_t timestamp, const std::string& message,
                                size_t num_uncompressed_bytes) {
        // Encode message and add components to dictionaries
        std::vector<encoded_variable_t> encoded_vars;
        std::vector<variable_dictionary_id_t> var_ids;
        EncodedVariableInterpreter::encode_and_add_to_dictionary(message, m_logtype_dict_entry,
                                                                 m_var_dict, encoded_vars,
                                                                 var_ids);
        logtype_dictionary_id_t logtype_id;
        m_logtype_dict.add_entry(m_logtype_dict_entry, logtype_id);
        size_t offset = m_glt_segment.append_to_segment(logtype_id, timestamp, m_file_id, encoded_vars);
        // Issue: the offset of var_segments is per file based. However, we still need to add the offset of segments.
        // the offset of segment is not known because we don't know if the segment should be timestamped...
        // Here for simplicity, we add the segment offset back when we close the file
        m_glt_file->write_encoded_msg(timestamp, logtype_id, offset, var_ids,
                                      num_uncompressed_bytes, encoded_vars.size());
        // Update segment indices
        m_logtype_ids_in_segment.insert(logtype_id);
        m_var_ids_in_segment.insert_all(var_ids);
    }

    void GLTArchive::write_msg_using_schema (compressor_frontend::Token*& uncompressed_msg,
                                             uint32_t uncompressed_msg_pos,
                                             const bool has_delimiter,
                                             const bool has_timestamp) {
        epochtime_t timestamp = 0;
        TimestampPattern* timestamp_pattern = nullptr;
        if (has_timestamp) {
            size_t start;
            size_t end;
            timestamp_pattern = (TimestampPattern*)TimestampPattern::search_known_ts_patterns(
                    uncompressed_msg[0].get_string(), timestamp, start, end);
            if (old_ts_pattern != *timestamp_pattern) {
                change_ts_pattern(timestamp_pattern);
                old_ts_pattern = *timestamp_pattern;
            }
            assert(nullptr != timestamp_pattern);
        }
        if (get_data_size_of_dictionaries() >= m_target_data_size_of_dicts) {
            clp::split_file_and_archive(m_archive_user_config, m_path_for_compression, m_group_id,
                                        timestamp_pattern, *this);
        } else if (m_file->get_encoded_size_in_bytes() >= m_target_encoded_file_size) {
            clp::split_file(m_path_for_compression, m_group_id, timestamp_pattern, *this);
        }

        m_encoded_vars.clear();
        m_var_ids.clear();
        m_logtype_dict_entry.clear();

        size_t num_uncompressed_bytes = 0;
        // Timestamp is included in the uncompressed message size
        uint32_t start_pos = uncompressed_msg[0].m_start_pos;
        if (timestamp_pattern == nullptr) {
            start_pos = uncompressed_msg[1].m_start_pos;
        }
        uint32_t end_pos = uncompressed_msg[uncompressed_msg_pos - 1].m_end_pos;
        if (start_pos <= end_pos) {
            num_uncompressed_bytes = end_pos - start_pos;
        } else {
            num_uncompressed_bytes = *uncompressed_msg[0].m_buffer_size_ptr - start_pos + end_pos;
        }
        for (uint32_t i = 1; i < uncompressed_msg_pos; i++) {
            compressor_frontend::Token& token = uncompressed_msg[i];
            int token_type = token.m_type_ids->at(0);
            if (has_delimiter &&
                token_type != (int)compressor_frontend::SymbolID::TokenUncaughtStringID &&
                token_type != (int)compressor_frontend::SymbolID::TokenNewlineId) {
                m_logtype_dict_entry.add_constant(token.get_delimiter(), 0, 1);
                if (token.m_start_pos == *token.m_buffer_size_ptr - 1) {
                    token.m_start_pos = 0;
                } else {
                    token.m_start_pos++;
                }
            }
            switch (token_type) {
                case (int)compressor_frontend::SymbolID::TokenNewlineId:
                case (int)compressor_frontend::SymbolID::TokenUncaughtStringID: {
                    m_logtype_dict_entry.add_constant(token.get_string(), 0, token.get_length());
                    break;
                }
                case (int)compressor_frontend::SymbolID::TokenIntId: {
                    encoded_variable_t encoded_var;
                    if (!EncodedVariableInterpreter::convert_string_to_representable_integer_var(
                            token.get_string(), encoded_var)) {
                        variable_dictionary_id_t id;
                        m_var_dict.add_entry(token.get_string(), id);
                        encoded_var = EncodedVariableInterpreter::encode_var_dict_id(id);
                    }
                    m_logtype_dict_entry.add_non_double_var();
                    m_encoded_vars.push_back(encoded_var);
                    break;
                }
                case (int)compressor_frontend::SymbolID::TokenDoubleId: {
                    encoded_variable_t encoded_var;
                    if (!EncodedVariableInterpreter::convert_string_to_representable_double_var(
                            token.get_string(), encoded_var)) {
                        variable_dictionary_id_t id;
                        m_var_dict.add_entry(token.get_string(), id);
                        encoded_var = EncodedVariableInterpreter::encode_var_dict_id(id);
                        m_logtype_dict_entry.add_non_double_var();
                    } else {
                        m_logtype_dict_entry.add_double_var();
                    }
                    m_encoded_vars.push_back(encoded_var);
                    break;
                }
                default: {
                    // Variable string looks like a dictionary variable, so encode it as so
                    encoded_variable_t encoded_var;
                    variable_dictionary_id_t id;
                    m_var_dict.add_entry(token.get_string(), id);
                    encoded_var = EncodedVariableInterpreter::encode_var_dict_id(id);
                    m_var_ids.push_back(id);

                    m_logtype_dict_entry.add_non_double_var();
                    m_encoded_vars.push_back(encoded_var);
                    break;
                }
            }
        }
        if (!m_logtype_dict_entry.get_value().empty()) {
            logtype_dictionary_id_t logtype_id;
            m_logtype_dict.add_entry(m_logtype_dict_entry, logtype_id);
            size_t offset = m_glt_segment.append_to_segment(
                    logtype_id, timestamp, m_file_id, m_encoded_vars);
            m_glt_file->write_encoded_msg(timestamp, logtype_id, offset, m_var_ids,
                                          num_uncompressed_bytes, m_encoded_vars.size());

            // Update segment indices
            m_logtype_ids_in_segment.insert(logtype_id);
            m_var_ids_in_segment.insert_all(m_var_ids);
        }
    }

    void GLTArchive::append_file_to_segment () {
        if (m_file == nullptr) {
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }
        // Haiqi TODO: this open logic is counter intuitive for glt_segment
        // because the open happens after file content gets appended
        // to m_glt_segment.
        if (!m_message_order_table.is_open()) {
            m_glt_segment.open(m_segments_dir_path, m_next_segment_id,
                               m_compression_level, m_combine_threshold);
            m_message_order_table.open(m_segments_dir_path, m_next_segment_id,
                                       m_compression_level);
            m_next_segment_id++;
        }
        append_file_contents_to_segment(m_message_order_table,
                                        m_glt_segment,
                                        m_logtype_ids_in_segment,
                                        m_var_ids_in_segment,
                                        m_files_in_segment);

        // Make sure file pointer is nulled and cannot be accessed outside
        m_file = nullptr;
    }

    void GLTArchive::append_file_contents_to_segment (Segment& segment,
                                                      GLTSegment& glt_segment,
                                                      ArrayBackedPosIntSet<logtype_dictionary_id_t>& logtype_ids_in_segment,
                                                      ArrayBackedPosIntSet<variable_dictionary_id_t>& var_ids_in_segment,
                                                      std::vector<File*>& files_in_segment) {

        m_file->append_to_segment(m_logtype_dict, segment);
        files_in_segment.emplace_back(m_file);

        // Close current segment if its uncompressed size is greater than the target
        if (segment.get_uncompressed_size() + glt_segment.get_uncompressed_size() >=
            m_target_segment_uncompressed_size) {
            close_segment_and_persist_file_metadata(segment, glt_segment, files_in_segment,
                                                    logtype_ids_in_segment, var_ids_in_segment);
            logtype_ids_in_segment.clear();
            var_ids_in_segment.clear();
        }
    }

    void GLTArchive::close_segment_and_persist_file_metadata (Segment& on_disk_stream,
                                                              GLTSegment& glt_segment,
                                                              std::vector<File*>& files,
                                                              ArrayBackedPosIntSet<logtype_dictionary_id_t>& segment_logtype_ids,
                                                              ArrayBackedPosIntSet<variable_dictionary_id_t>& segment_var_ids) {
        auto segment_id = on_disk_stream.get_id();
        m_logtype_dict.index_segment(segment_id, segment_logtype_ids);
        m_var_dict.index_segment(segment_id, segment_var_ids);

        m_stable_size += on_disk_stream.get_compressed_size();

        on_disk_stream.close();
        glt_segment.close();
        // Have to call this function after closing the segments because
        // Variable segment currently only gets written to disk after closing the segment.
        m_stable_size += glt_segment.get_compressed_size();
#if FLUSH_TO_DISK_ENABLED
        // fsync segments directory to flush segment's directory entry
        if (fsync(m_segments_dir_fd) != 0) {
            SPDLOG_ERROR("Failed to fsync {}, errno={}", m_segments_dir_path.c_str(), errno);
            throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
        }
#endif

        // Flush dictionaries
        m_logtype_dict.write_header_and_flush_to_disk();
        m_var_dict.write_header_and_flush_to_disk();

        m_global_metadata_db->open();
        persist_file_metadata(files);
        update_metadata();
        m_global_metadata_db->close();

        for (auto file : files) {
            m_stable_uncompressed_size += file->get_num_uncompressed_bytes();
            delete file;
        }
        files.clear();
    }

    size_t GLTArchive::get_stable_size () const {
        return Archive::get_stable_size() + m_filename_dict_writer.get_pos();
    }
}