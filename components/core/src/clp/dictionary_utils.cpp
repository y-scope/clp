#include "dictionary_utils.hpp"

#include "FileReader.hpp"

namespace clp {
uint64_t read_dictionary_header(FileReader& file_reader) {
    auto dictionary_file_reader_pos = file_reader.get_pos();
    file_reader.seek_from_begin(0);
    uint64_t num_dictionary_entries{0};
    file_reader.read_numeric_value(num_dictionary_entries, false);
    file_reader.seek_from_begin(dictionary_file_reader_pos);
    return num_dictionary_entries;
}

uint64_t read_segment_index_header(FileReader& file_reader) {
    // Read segment index header
    auto segment_index_file_reader_pos = file_reader.get_pos();
    file_reader.seek_from_begin(0);
    uint64_t num_segments{0};
    file_reader.read_numeric_value(num_segments, false);
    file_reader.seek_from_begin(segment_index_file_reader_pos);
    return num_segments;
}
}  // namespace clp
