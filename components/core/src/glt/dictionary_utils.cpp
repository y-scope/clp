#include "dictionary_utils.hpp"

namespace glt {
void open_dictionary_for_reading(
        std::string const& dictionary_path,
        std::string const& segment_index_path,
        size_t decompressor_file_read_buffer_capacity,
        FileReader& dictionary_file_reader,
        streaming_compression::Decompressor& dictionary_decompressor,
        FileReader& segment_index_file_reader,
        streaming_compression::Decompressor& segment_index_decompressor
) {
    dictionary_file_reader.open(dictionary_path);
    // Skip header
    dictionary_file_reader.seek_from_begin(sizeof(uint64_t));
    // Open decompressor
    dictionary_decompressor.open(dictionary_file_reader, decompressor_file_read_buffer_capacity);

    segment_index_file_reader.open(segment_index_path);
    // Skip header
    segment_index_file_reader.seek_from_begin(sizeof(uint64_t));
    // Open decompressor
    segment_index_decompressor.open(
            segment_index_file_reader,
            decompressor_file_read_buffer_capacity
    );
}

uint64_t read_dictionary_header(FileReader& file_reader) {
    auto dictionary_file_reader_pos = file_reader.get_pos();
    file_reader.seek_from_begin(0);
    uint64_t num_dictionary_entries;
    file_reader.read_numeric_value(num_dictionary_entries, false);
    file_reader.seek_from_begin(dictionary_file_reader_pos);
    return num_dictionary_entries;
}

uint64_t read_segment_index_header(FileReader& file_reader) {
    // Read segment index header
    auto segment_index_file_reader_pos = file_reader.get_pos();
    file_reader.seek_from_begin(0);
    uint64_t num_segments;
    file_reader.read_numeric_value(num_segments, false);
    file_reader.seek_from_begin(segment_index_file_reader_pos);
    return num_segments;
}
}  // namespace glt
