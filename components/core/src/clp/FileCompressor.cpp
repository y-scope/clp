#include "FileCompressor.hpp"

// C++ standard libraries
#include <algorithm>
#include <iostream>
#include <set>

// Boost libraries
#include <boost/filesystem/path.hpp>

// libarchive
#include <archive_entry.h>

// Project headers
#include "../Profiler.hpp"
#include "../IrMessageParser.hpp"
#include "utils.hpp"
#include "../ffi/ir_stream/protocol_constants.hpp"

// json
#include "../../../submodules/json/single_include/nlohmann/json.hpp"

using std::cout;
using std::endl;
using std::set;
using std::string;
using std::vector;

// Local prototypes
/**
 * Computes empty directories as directories - parent_directories and adds them to the given archive
 * @param directories
 * @param parent_directories
 * @param parent_path Path that should be the parent of all added directories
 * @param archive
 */
static void compute_and_add_empty_directories (const set<string>& directories, const set<string>& parent_directories,
                                               const boost::filesystem::path& parent_path, streaming_archive::writer::Archive& archive);

/**
 * Writes the given message to the given encoded file
 * @param msg
 * @param archive
 * @param file
 */
static void write_message_to_encoded_file (const ParsedMessage& msg, streaming_archive::writer::Archive& archive);

static bool is_ir_encoded(ReaderInterface& reader, bool& is_four_bytes_encoded);

static void compute_and_add_empty_directories (const set<string>& directories, const set<string>& parent_directories,
                                               const boost::filesystem::path& parent_path, streaming_archive::writer::Archive& archive)
{
    // Determine empty directories by subtracting parent directories
    vector<string> empty_directories;
    auto directories_ix = directories.cbegin();
    for (auto parent_directories_ix = parent_directories.cbegin();
         directories.cend() != directories_ix && parent_directories.cend() != parent_directories_ix;)
    {
        const auto& directory = *directories_ix;
        const auto& parent_directory = *parent_directories_ix;

        if (directory < parent_directory) {
            auto boost_path_for_compression = parent_path / directory;
            empty_directories.emplace_back(boost_path_for_compression.string());
            ++directories_ix;
        } else if (directory == parent_directory) {
            ++directories_ix;
            ++parent_directories_ix;
        } else {
            ++parent_directories_ix;
        }
    }
    for (; directories.cend() != directories_ix; ++directories_ix) {
        auto boost_path_for_compression = parent_path / *directories_ix;
        empty_directories.emplace_back(boost_path_for_compression.string());
    }
    archive.add_empty_directories(empty_directories);
}

static void write_message_to_encoded_file (const ParsedMessage& msg, streaming_archive::writer::Archive& archive) {
    if (msg.has_ts_patt_changed()) {
        archive.change_ts_pattern(msg.get_ts_patt());
    }

    archive.write_msg(msg.get_ts(), msg.get_content(), msg.get_orig_num_bytes());
}

static void write_ir_message_to_encoded_file (const ParsedIrMessage& msg, streaming_archive::writer::Archive& archive) {
    archive.write_msg(msg);
}

namespace clp {
    bool FileCompressor::compress_file (size_t target_data_size_of_dicts, streaming_archive::writer::Archive::UserConfig& archive_user_config,
                                        size_t target_encoded_file_size, const FileToCompress& file_to_compress,
                                        streaming_archive::writer::Archive& archive_writer, bool use_heuristic) {
        std::string file_name = std::filesystem::canonical(file_to_compress.get_path()).string();

        PROFILER_SPDLOG_INFO("Start parsing {}", file_name)
        Profiler::start_continuous_measurement<Profiler::ContinuousMeasurementIndex::ParseLogFile>();

        m_file_reader.open(file_to_compress.get_path());

        // Check that file is UTF-8 encoded
        if (auto error_code = m_file_reader.peek_buffered_data(cUtf8ValidationBufCapacity,
                                                               m_utf8_validation_buf,
                                                               m_utf8_validation_buf_length);
            ErrorCode_Success != error_code && ErrorCode_EndOfFile != error_code) {
            SPDLOG_ERROR("Failed to peek data from {}, errno={}",
                         file_to_compress.get_path().c_str(), errno);
            return error_code;
        }
        bool succeeded = true;
        if (is_utf8_sequence(m_utf8_validation_buf_length, m_utf8_validation_buf)) {
            if (use_heuristic) {
                parse_and_encode_with_heuristic(target_data_size_of_dicts, archive_user_config, target_encoded_file_size,
                                                file_to_compress.get_path_for_compression(),
                                                file_to_compress.get_group_id(), archive_writer, m_file_reader);
            } else {
                parse_and_encode(target_data_size_of_dicts, archive_user_config, target_encoded_file_size,
                                 file_to_compress.get_path_for_compression(),
                                 file_to_compress.get_group_id(), archive_writer, m_file_reader);
            }
        } else {
            if (false == try_compressing_as_archive(target_data_size_of_dicts, archive_user_config, target_encoded_file_size, file_to_compress,
                                                    archive_writer, use_heuristic))
            {
                succeeded = false;
            }
        }

        m_file_reader.close();

        Profiler::stop_continuous_measurement<Profiler::ContinuousMeasurementIndex::ParseLogFile>();
        LOG_CONTINUOUS_MEASUREMENT(Profiler::ContinuousMeasurementIndex::ParseLogFile)
        PROFILER_SPDLOG_INFO("Done parsing {}", file_name)

        return succeeded;
    }

    void FileCompressor::parse_and_encode (size_t target_data_size_of_dicts, streaming_archive::writer::Archive::UserConfig& archive_user_config,
                                           size_t target_encoded_file_size, const string& path_for_compression, group_id_t group_id,
                                           streaming_archive::writer::Archive& archive_writer, ReaderInterface& reader)
    {
        archive_writer.m_target_data_size_of_dicts = target_data_size_of_dicts;
        archive_writer.m_archive_user_config = archive_user_config;
        archive_writer.m_path_for_compression = path_for_compression;
        archive_writer.m_group_id = group_id;
        archive_writer.m_target_encoded_file_size = target_encoded_file_size;
        // Open compressed file
        archive_writer.create_and_open_file(path_for_compression, group_id, m_uuid_generator(), 0);
        // TODO: decide what to actually do about this
        // for now reset reader rather than try reading m_utf8_validation_buf as it would be
        // very awkward to combine sources to/in the parser
        m_log_parser->set_archive_writer_ptr(&archive_writer);
        m_log_parser->get_archive_writer_ptr()->old_ts_pattern.clear();
        try {
            m_log_parser->parse(reader);
        } catch (std::string const err) {
            if (err.find("Lexer failed to find a match after checking entire buffer") != std::string::npos) {
                close_file_and_append_to_segment(archive_writer);
                SPDLOG_ERROR(err);
            } else {
                throw (err);
            }
        }
        // TODO: separate variables from static text
        //Stopwatch close_file_watch("close_file_watch");
        //close_file_watch.start();
        close_file_and_append_to_segment(archive_writer);
        // archive_writer_config needs to persist between files
        archive_user_config = archive_writer.m_archive_user_config;
        //close_file_watch.stop();
        //close_file_watch.print();
    }

    void FileCompressor::parse_and_encode_with_heuristic (size_t target_data_size_of_dicts, streaming_archive::writer::Archive::UserConfig& archive_user_config,
                                                          size_t target_encoded_file_size, const string& path_for_compression, group_id_t group_id,
                                                          streaming_archive::writer::Archive& archive_writer, ReaderInterface& reader)
    {
        m_parsed_message.clear();

        // Open compressed file
        archive_writer.create_and_open_file(path_for_compression, group_id, m_uuid_generator(), 0);

        // Parse content from file
        while (m_message_parser.parse_next_message(true, reader, m_parsed_message)) {
            if (archive_writer.get_data_size_of_dictionaries() >= target_data_size_of_dicts) {
                split_file_and_archive(archive_user_config, path_for_compression, group_id, m_parsed_message.get_ts_patt(), archive_writer);
            } else if (archive_writer.get_file().get_encoded_size_in_bytes() >= target_encoded_file_size) {
                split_file(path_for_compression, group_id, m_parsed_message.get_ts_patt(), archive_writer);
            }

            write_message_to_encoded_file(m_parsed_message, archive_writer);
        }

        close_file_and_append_to_segment(archive_writer);
    }

    bool FileCompressor::try_compressing_as_archive (size_t target_data_size_of_dicts, streaming_archive::writer::Archive::UserConfig& archive_user_config,
                                                     size_t target_encoded_file_size, const FileToCompress& file_to_compress,
                                                     streaming_archive::writer::Archive& archive_writer, bool use_heuristic)
    {
        auto file_boost_path = boost::filesystem::path(file_to_compress.get_path_for_compression());
        auto parent_boost_path = file_boost_path.parent_path();

        // Determine path without extension (used if file is a single compressed file, e.g., syslog.gz -> syslog)
        std::string filename_if_compressed;
        if (file_boost_path.has_stem()) {
            filename_if_compressed = file_boost_path.stem().string();
        } else {
            filename_if_compressed = file_boost_path.filename().string();
        }

        // Check if it's an archive
        auto error_code = m_libarchive_reader.try_open(m_file_reader, filename_if_compressed);
        if (ErrorCode_Success != error_code) {
            SPDLOG_ERROR("Cannot compress {} - failed to open with libarchive.", file_to_compress.get_path().c_str());
            return false;
        }

        // Compress each file and directory in the archive
        bool succeeded = true;
        set<string> directories;
        set<string> parent_directories;
        while (true) {
            error_code = m_libarchive_reader.try_read_next_header();
            if (ErrorCode_Success != error_code) {
                if (ErrorCode_EndOfFile == error_code) {
                    break;
                }
                SPDLOG_ERROR("Failed to read entry in {}.", file_to_compress.get_path().c_str());
                succeeded = false;
                break;
            }

            // Determine what type of file it is
            auto file_type = m_libarchive_reader.get_entry_file_type();
            if (AE_IFREG != file_type) {
                if (AE_IFDIR == file_type) {
                    // Trim trailing slash
                    string directory_path(m_libarchive_reader.get_path());
                    directory_path.resize(directory_path.length() - 1);

                    directories.emplace(directory_path);

                    auto directory_parent_path = boost::filesystem::path(directory_path).parent_path().string();
                    if (false == directory_parent_path.empty()) {
                        parent_directories.emplace(directory_parent_path);
                    }
                } // else ignore irregular files
                continue;
            }
            auto file_parent_path = boost::filesystem::path(m_libarchive_reader.get_path()).parent_path().string();
            if (false == file_parent_path.empty()) {
                parent_directories.emplace(file_parent_path);
            }

            if (archive_writer.get_data_size_of_dictionaries() >= target_data_size_of_dicts) {
                split_archive(archive_user_config, archive_writer);
            }

            m_libarchive_reader.open_file_reader(m_libarchive_file_reader);
            error_code = m_libarchive_file_reader.peek_data_block(cUtf8ValidationBufCapacity,
                                                                  m_utf8_validation_buf,
                                                                  m_utf8_validation_buf_length);
            // Check that file is UTF-8 encoded
            if (ErrorCode_Success != error_code) {
                if (ErrorCode_EndOfFile != error_code) {
                    SPDLOG_ERROR("Failed to peek data from {}, errno={}",
                                 file_to_compress.get_path().c_str(), errno);
                    m_libarchive_file_reader.close();
                    succeeded = false;
                    continue;
                }
            }
            bool is_four_bytes_encoded {false};
            if (is_utf8_sequence(m_utf8_validation_buf_length, m_utf8_validation_buf)) {
                auto boost_path_for_compression = parent_boost_path / m_libarchive_reader.get_path();
                if (use_heuristic) {
                    parse_and_encode_with_heuristic(target_data_size_of_dicts, archive_user_config, target_encoded_file_size,
                                                    boost_path_for_compression.string(), file_to_compress.get_group_id(), archive_writer,
                                                    m_libarchive_file_reader);
                } else {
                    parse_and_encode(target_data_size_of_dicts, archive_user_config, target_encoded_file_size, boost_path_for_compression.string(),
                                     file_to_compress.get_group_id(), archive_writer, m_libarchive_file_reader);
                }
            } else if (is_ir_encoded(m_libarchive_file_reader, is_four_bytes_encoded)) {
                auto boost_path_for_compression = parent_boost_path / m_libarchive_reader.get_path();
                if (false == try_compressing_as_ir(target_data_size_of_dicts, archive_user_config, target_encoded_file_size, boost_path_for_compression.string(),
                                                   file_to_compress.get_group_id(), archive_writer, m_libarchive_file_reader, is_four_bytes_encoded)) {
                    SPDLOG_ERROR("SOME Error message to be printed");

                }

            } else {
                SPDLOG_ERROR("Cannot compress {} - not UTF-8 or IR encoded.", m_libarchive_reader.get_path());
                succeeded = false;
            }

            m_libarchive_file_reader.close();
        }
        compute_and_add_empty_directories(directories, parent_directories, parent_boost_path, archive_writer);

        m_libarchive_reader.close();

        return succeeded;
    }

    bool FileCompressor::try_compressing_as_ir (size_t target_data_size_of_dicts,
                                                streaming_archive::writer::Archive::UserConfig& archive_user_config,
                                                size_t target_encoded_file_size,
                                                const std::string& path_for_compression,
                                                group_id_t group_id,
                                                streaming_archive::writer::Archive& archive_writer,
                                                ReaderInterface& reader,
                                                bool is_four_bytes_encoded)
    {
        printf("IR compressing\n");
        m_parsed_ir_message.clear();

        // Open compressed file
        if (!is_four_bytes_encoded) {
            SPDLOG_ERROR("not supported yet");
            throw;
        }

        // Decode and parse metadata
        ffi::ir_stream::encoded_tag_t metadata_type;
        std::vector<uint8_t> json_metadata_vec;
        epochtime_t reference_ts;

        if (ffi::ir_stream::IRErrorCode_Success != ffi::ir_stream::decode_preamble(reader, metadata_type, json_metadata_vec)) {
            SPDLOG_ERROR("Failed to parse metadata");
            return false;
        }

        if (ffi::ir_stream::cProtocol::Metadata::EncodingJson != metadata_type) {
            SPDLOG_ERROR("Unexpected metadata type");
            return false;
        }

        std::string_view json_metadata {reinterpret_cast<const char*>(json_metadata_vec.data()),
                                        json_metadata_vec.size()};
        try {
            auto metadata_json = nlohmann::json::parse(json_metadata);
            string version = metadata_json.at(ffi::ir_stream::cProtocol::Metadata::VersionKey);
            if (version != ffi::ir_stream::cProtocol::Metadata::VersionValue) {
                SPDLOG_ERROR("Unsupported version");
                return false;
            }

            // assume that we only need reference timestamp
            reference_ts = std::stoll(metadata_json.at(
                    ffi::ir_stream::cProtocol::Metadata::ReferenceTimestampKey).get<string>());
        } catch (const nlohmann::json::parse_error& e) {
            SPDLOG_ERROR("Failed to parse json metadata");
            return false;
        }

        archive_writer.create_and_open_file(path_for_compression, group_id, m_uuid_generator(), 0);

        // TODO: use a mocked ts pattern for this now
        TimestampPattern mocked_ts_pattern(0, "%Y-%m-%dT%H:%M:%S.%3");
        archive_writer.change_ts_pattern(&mocked_ts_pattern);
        m_parsed_ir_message.set_ts_pattern(&mocked_ts_pattern);

        while (true) {
            if (false == IrMessageParser::parse_four_bytes_encoded_message(reader, m_parsed_ir_message, reference_ts)) {
                break;
            }
            if (archive_writer.get_data_size_of_dictionaries() >= target_data_size_of_dicts) {
                split_file_and_archive(archive_user_config, path_for_compression, group_id, &mocked_ts_pattern, archive_writer);
            } else if (archive_writer.get_file().get_encoded_size_in_bytes() >= target_encoded_file_size) {
                split_file(path_for_compression, group_id, &mocked_ts_pattern, archive_writer);
            }
            write_ir_message_to_encoded_file(m_parsed_ir_message, archive_writer);
        }
        close_file_and_append_to_segment(archive_writer);

        return true;
    }
}

static bool is_ir_encoded (ReaderInterface& reader, bool& is_four_bytes_encoded) {
    if (ffi::ir_stream::IRErrorCode_Success !=
            ffi::ir_stream::get_encoding_type(reader, is_four_bytes_encoded)) {
        return false;
    }
    return true;
}