#ifndef CLP_FILECOMPRESSOR_HPP
#define CLP_FILECOMPRESSOR_HPP

// Boost libraries
#include <boost/uuid/random_generator.hpp>

// Project headers
#include "../FileReader.hpp"
#include "../LibarchiveFileReader.hpp"
#include "../LibarchiveReader.hpp"
#include "../MessageParser.hpp"
#include "../ParsedMessage.hpp"
#include "../streaming_archive/writer/Archive.hpp"
#include "FileToCompress.hpp"
#include "../compressor_frontend/LogParser.hpp"

namespace clp {
    constexpr size_t cUtf8ValidationBufCapacity = 4096;

    /**
     * Class to parse and compress a file into a streaming archive
     */
    class FileCompressor {
    public:
        // Constructors
        FileCompressor (boost::uuids::random_generator& uuid_generator, std::unique_ptr<compressor_frontend::LogParser> log_parser) : m_uuid_generator(
                uuid_generator), m_log_parser(std::move(log_parser)) {}

        // Methods
        /**
         * Compresses a file with the given path into the archive
         * @param target_data_size_of_dicts
         * @param archive_user_config
         * @param target_encoded_file_size
         * @param file_to_compress
         * @param archive_writer
         * @return true if the file was compressed successfully, false otherwise
         */
        bool compress_file (size_t target_data_size_of_dicts,
                            streaming_archive::writer::Archive::UserConfig& archive_user_config,
                            size_t target_encoded_file_size, const FileToCompress& file_to_compress,
                            streaming_archive::writer::Archive& archive_writer, bool use_heuristic);

    private:
        // Methods
        /**
         * Parses and encodes content from the given reader into the given archive_writer
         * @param target_data_size_of_dicts
         * @param archive_user_config
         * @param target_encoded_file_size
         * @param path_for_compression
         * @param group_id
         * @param archive_writer
         * @param reader
         */
        void parse_and_encode (size_t target_data_size_of_dicts, streaming_archive::writer::Archive::UserConfig& archive_user_config,
                               size_t target_encoded_file_size, const std::string& path_for_compression, group_id_t group_id,
                               streaming_archive::writer::Archive& archive_writer, ReaderInterface& reader);

        void parse_and_encode_with_heuristic (size_t target_data_size_of_dicts, streaming_archive::writer::Archive::UserConfig& archive_user_config,
                                              size_t target_encoded_file_size, const std::string& path_for_compression, group_id_t group_id,
                                              streaming_archive::writer::Archive& archive_writer, ReaderInterface& reader);

        /**
         * Tries to compress the given file as if it were a generic archive_writer
         * @param target_data_size_of_dicts
         * @param archive_user_config
         * @param target_encoded_file_size
         * @param file_to_compress
         * @param archive_writer
         * @param use_heuristic
         * @return true if all files were compressed successfully, false otherwise
         */
        bool try_compressing_as_archive (size_t target_data_size_of_dicts, streaming_archive::writer::Archive::UserConfig& archive_user_config,
                                         size_t target_encoded_file_size, const FileToCompress& file_to_compress,
                                         streaming_archive::writer::Archive& archive_writer, bool use_heuristic);

        // Variables
        boost::uuids::random_generator& m_uuid_generator;
        FileReader m_file_reader;
        LibarchiveReader m_libarchive_reader;
        LibarchiveFileReader m_libarchive_file_reader;
        char m_utf8_validation_buf[cUtf8ValidationBufCapacity];
        size_t m_utf8_validation_buf_length;
        MessageParser m_message_parser;
        ParsedMessage m_parsed_message;
        std::unique_ptr<compressor_frontend::LogParser> m_log_parser;
    };
}

#endif // CLP_FILECOMPRESSOR_HPP
