#ifndef CLP_DICTIONARY_UTILS_HPP
#define CLP_DICTIONARY_UTILS_HPP

#include "FileReader.hpp"
#include "streaming_compression/Decompressor.hpp"

namespace clp {
uint64_t read_dictionary_header(FileReader& file_reader);

uint64_t read_segment_index_header(FileReader& file_reader);
}  // namespace clp

#endif  // CLP_DICTIONARY_UTILS_HPP
