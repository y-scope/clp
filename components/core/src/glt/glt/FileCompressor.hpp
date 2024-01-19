#ifndef GLT_GLT_FILECOMPRESSOR_HPP
#define GLT_GLT_FILECOMPRESSOR_HPP

#include <system_error>

#include <boost/uuid/random_generator.hpp>

#include "../BufferedFileReader.hpp"
#include "../ir/LogEventDeserializer.hpp"
#include "../LibarchiveFileReader.hpp"
#include "../LibarchiveReader.hpp"
#include "../MessageParser.hpp"
#include "../ParsedMessage.hpp"
#include "../streaming_archive/writer/Archive.hpp"
#include "FileToCompress.hpp"

namespace glt::glt {
/**
 * Class to parse and compress a file into a streaming archive
 */
class FileCompressor {
public:
    // Constructors
    FileCompressor(boost::uuids::random_generator& uuid_generator)
            : m_uuid_generator(uuid_generator) {}

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
    bool compress_file(
            size_t target_data_size_of_dicts,
            streaming_archive::writer::Archive::UserConfig& archive_user_config,
            size_t target_encoded_file_size,
            FileToCompress const& file_to_compress,
            streaming_archive::writer::Archive& archive_writer
    );

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
    void parse_and_encode_with_heuristic(
            size_t target_data_size_of_dicts,
            streaming_archive::writer::Archive::UserConfig& archive_user_config,
            size_t target_encoded_file_size,
            std::string const& path_for_compression,
            group_id_t group_id,
            streaming_archive::writer::Archive& archive_writer,
            ReaderInterface& reader
    );

    /**
     * Tries to compress the given file as if it were a generic archive_writer
     * @param target_data_size_of_dicts
     * @param archive_user_config
     * @param target_encoded_file_size
     * @param file_to_compress
     * @param archive_writer
     * @return true if all files were compressed successfully, false otherwise
     */
    bool try_compressing_as_archive(
            size_t target_data_size_of_dicts,
            streaming_archive::writer::Archive::UserConfig& archive_user_config,
            size_t target_encoded_file_size,
            FileToCompress const& file_to_compress,
            streaming_archive::writer::Archive& archive_writer
    );

    // Variables
    boost::uuids::random_generator& m_uuid_generator;
    BufferedFileReader m_file_reader;
    LibarchiveReader m_libarchive_reader;
    LibarchiveFileReader m_libarchive_file_reader;
    MessageParser m_message_parser;
    ParsedMessage m_parsed_message;
};
}  // namespace glt::glt

#endif  // GLT_GLT_FILECOMPRESSOR_HPP
