#ifndef GLT_DICTIONARY_UTILS_HPP
#define GLT_DICTIONARY_UTILS_HPP

#include <string>

#include "FileReader.hpp"
#include "streaming_compression/Decompressor.hpp"

namespace glt {
void open_dictionary_for_reading(
        std::string const& dictionary_path,
        std::string const& segment_index_path,
        size_t decompressor_file_read_buffer_capacity,
        FileReader& dictionary_file_reader,
        streaming_compression::Decompressor& dictionary_decompressor,
        FileReader& segment_index_file_reader,
        streaming_compression::Decompressor& segment_index_decompressor
);

uint64_t read_dictionary_header(FileReader& file_reader);

uint64_t read_segment_index_header(FileReader& file_reader);
}  // namespace glt

#endif  // GLT_DICTIONARY_UTILS_HPP
