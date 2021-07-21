#include "VariableDictionaryWriter.hpp"

// spdlog
#include <spdlog/spdlog.h>

// Project headers
#include "dictionary_utils.hpp"

using std::string;

void VariableDictionaryWriter::open_and_preload (const string& dictionary_path, const string& segment_index_path, variable_dictionary_id_t max_id) {
    if (m_is_open) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }

    m_max_id = max_id;

    FileReader dictionary_file_reader;
    streaming_compression::zstd::Decompressor dictionary_decompressor;
    FileReader segment_index_file_reader;
    streaming_compression::zstd::Decompressor segment_index_decompressor;
    constexpr size_t cDecompressorFileReadBufferCapacity = 64 * 1024; // 64 KB
    open_dictionary_for_reading(dictionary_path, segment_index_path, cDecompressorFileReadBufferCapacity, dictionary_file_reader, dictionary_decompressor,
                                segment_index_file_reader, segment_index_decompressor);

    auto num_dictionary_entries = read_dictionary_header(dictionary_file_reader);

    // Read new dictionary entries
    variable_dictionary_id_t id;
    VariableDictionaryEntry var_dict_entry;
    for (size_t i = 0; i < num_dictionary_entries; ++i) {
        var_dict_entry.read_from_file(dictionary_decompressor);
        add_occurrence(var_dict_entry.get_value(), id);
    }
    m_uncommitted_entries.clear();

    segment_index_decompressor.close();
    segment_index_file_reader.close();
    dictionary_decompressor.close();
    dictionary_file_reader.close();

    m_dictionary_file_writer.open(dictionary_path, FileWriter::OpenMode::CREATE_IF_NONEXISTENT_FOR_SEEKABLE_WRITING);
    // Open compressor
    m_dictionary_compressor.open(m_dictionary_file_writer);

    m_segment_index_file_writer.open(segment_index_path, FileWriter::OpenMode::CREATE_IF_NONEXISTENT_FOR_SEEKABLE_WRITING);
    // Open compressor
    m_segment_index_compressor.open(m_segment_index_file_writer);

    m_is_open = true;
}

bool VariableDictionaryWriter::add_occurrence (const string& value, variable_dictionary_id_t& id) {
    bool new_entry = false;

    const auto ix = m_value_to_entry.find(value);
    if (m_value_to_entry.end() != ix) {
        id = ix->second->get_id();
    } else {
        // Entry doesn't exist so create it

        if (m_next_id > m_max_id) {
            SPDLOG_ERROR("VariableDictionaryWriter ran out of IDs.");
            throw OperationFailed(ErrorCode_OutOfBounds, __FILENAME__, __LINE__);
        }

        // Assign ID
        id = m_next_id;
        ++m_next_id;

        // Insert the ID obtained from the database into the dictionary
        auto* entry = new VariableDictionaryEntry(value, id);
        m_value_to_entry[value] = entry;
        m_id_to_entry[id] = entry;

        // Mark ID as dirty
        m_uncommitted_entries.emplace_back(entry);

        new_entry = true;

        // TODO: This doesn't account for the segment index that's constantly updated
        m_data_size += entry->get_data_size();
    }
    return new_entry;
}
