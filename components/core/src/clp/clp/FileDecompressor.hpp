#ifndef CLP_CLP_FILEDECOMPRESSOR_HPP
#define CLP_CLP_FILEDECOMPRESSOR_HPP

#include <cstddef>
#include <string>

#include "../FileWriter.hpp"
#include "../streaming_archive/MetadataDB.hpp"
#include "../streaming_archive/reader/Archive.hpp"
#include "../streaming_archive/reader/File.hpp"
#include "../streaming_archive/reader/Message.hpp"

namespace clp::clp {
/**
 * Class to hold the data structures that are used to decompress files rather than recreating them
 * within the decompression function or passing them as parameters.
 */
class FileDecompressor {
public:
    // Methods
    bool decompress_file(
            streaming_archive::MetadataDB::FileIterator const& file_metadata_ix,
            std::string const& output_dir,
            streaming_archive::reader::Archive& archive_reader,
            std::unordered_map<std::string, std::string>& temp_path_to_final_path
    );

    template <typename IrOutputHandler>
    auto decompress_to_ir(
        streaming_archive::MetadataDB::FileIterator const& file_metadata_ix,
        streaming_archive::reader::Archive& archive_reader,
        IrOutputHandler ir_output_handler,
        std::string const& temp_output_dir,
        size_t ir_target_size
    ) -> bool;

private:
    // Variables
    FileWriter m_decompressed_file_writer;
    streaming_archive::reader::File m_encoded_file;
    streaming_archive::reader::Message m_encoded_message;
    std::string m_decompressed_message;
};
};  // namespace clp::clp

#include "FileDecompressor.inc"

#endif  // CLP_CLP_FILEDECOMPRESSOR_HPP
