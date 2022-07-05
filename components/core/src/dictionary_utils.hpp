#ifndef DICTIONARY_UTILS_HPP
#define DICTIONARY_UTILS_HPP

// C++ standard libraries
#include <string>

// Project headers
#include "FileReader.hpp"
#include "streaming_compression/Decompressor.hpp"

void open_dictionary_for_reading (const std::string& dictionary_path, const std::string& segment_index_path, size_t decompressor_file_read_buffer_capacity,
                                  FileReader& dictionary_file_reader, streaming_compression::Decompressor& dictionary_decompressor,
                                  FileReader& segment_index_file_reader, streaming_compression::Decompressor& segment_index_decompressor);

uint64_t read_dictionary_header (FileReader& file_reader);

uint64_t read_segment_index_header (FileReader& file_reader);

#endif // DICTIONARY_UTILS_HPP
