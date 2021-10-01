#ifndef DICTIONARYWRITER_HPP
#define DICTIONARYWRITER_HPP

// C++ standard libraries
#include <unordered_map>
#include <unordered_set>
#include <vector>

// spdlog
#include <spdlog/spdlog.h>

// Project headers
#include "Defs.h"
#include "FileWriter.hpp"
#include "streaming_compression/zstd/Compressor.hpp"
#include "TraceableException.hpp"

/**
 * Template class for performing operations on dictionaries and writing them to disk
 * @tparam DictionaryIdType
 * @tparam EntryType
 */
template <typename DictionaryIdType, typename EntryType>
class DictionaryWriter {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "DictionaryWriter operation failed";
        }
    };

    // Constructors
    DictionaryWriter () : m_is_open(false) {}

    ~DictionaryWriter ();

    // Methods
    /**
     * Opens dictionary for writing
     * @param dictionary_path
     * @param segment_index_path
     */
    void open (const std::string& dictionary_path, const std::string& segment_index_path, DictionaryIdType max_id);
    /**
     * Closes the dictionary
     */
    void close ();

    /**
     * Gets the entry with the given ID
     * @param id
     * @return Pointer to the entry with the given ID
     */
    const EntryType* get_entry (DictionaryIdType id) const;

    /**
     * Writes uncommitted dictionary entries to file
     */
    void write_uncommitted_entries_to_disk ();

    /**
     * Adds the given segment and IDs to the segment index
     * @param segment_id
     * @param ids
     */
    void index_segment (segment_id_t segment_id, const std::unordered_set<DictionaryIdType>& ids);

    /**
     * Gets the size of the dictionary when it is stored on disk
     * @return Size in bytes
     */
    size_t get_on_disk_size () const { return m_dictionary_file_writer.get_pos() + m_segment_index_file_writer.get_pos(); }

    /**
     * Gets the size (in-memory) of the data contained in the dictionary
     * @return
     */
    size_t get_data_size () const { return m_data_size; }

protected:
    // Types
    typedef std::unordered_map<std::string, EntryType*> value_to_entry_t;
    typedef std::unordered_map<DictionaryIdType, EntryType*> id_to_entry_t;

    // Variables
    bool m_is_open;

    // Variables related to on-disk storage
    std::vector<EntryType*> m_uncommitted_entries;
    FileWriter m_dictionary_file_writer;
    streaming_compression::zstd::Compressor m_dictionary_compressor;
    FileWriter m_segment_index_file_writer;
    streaming_compression::zstd::Compressor m_segment_index_compressor;
    size_t m_num_segments_in_index;

    value_to_entry_t m_value_to_entry;
    id_to_entry_t  m_id_to_entry;
    DictionaryIdType m_next_id;
    DictionaryIdType m_max_id;

    // Size (in-memory) of the data contained in the dictionary
    size_t m_data_size;
};

template <typename DictionaryIdType, typename EntryType>
DictionaryWriter<DictionaryIdType, EntryType>::~DictionaryWriter () {
    if (false == m_uncommitted_entries.empty()) {
        SPDLOG_ERROR("DictionaryWriter contains uncommitted entries while being destroyed - possible data loss.");
    }
    for (const auto& value_entry_pair : m_value_to_entry) {
        delete value_entry_pair.second;
    }
}

template <typename DictionaryIdType, typename EntryType>
void DictionaryWriter<DictionaryIdType, EntryType>::open (const std::string& dictionary_path, const std::string& segment_index_path, DictionaryIdType max_id) {
    if (m_is_open) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }

    m_dictionary_file_writer.open(dictionary_path, FileWriter::OpenMode::CREATE_FOR_WRITING);
    // Write header
    m_dictionary_file_writer.write_numeric_value<uint64_t>(0);
    // Open compressor
    m_dictionary_compressor.open(m_dictionary_file_writer);

    m_segment_index_file_writer.open(segment_index_path, FileWriter::OpenMode::CREATE_FOR_WRITING);
    // Write header
    m_segment_index_file_writer.write_numeric_value<uint64_t>(0);
    // Open compressor
    m_segment_index_compressor.open(m_segment_index_file_writer);
    m_num_segments_in_index = 0;

    m_next_id = 0;
    m_max_id = max_id;

    m_data_size = 0;

    m_is_open = true;
}

template <typename DictionaryIdType, typename EntryType>
void DictionaryWriter<DictionaryIdType, EntryType>::close () {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    write_uncommitted_entries_to_disk();
    m_segment_index_compressor.close();
    m_segment_index_file_writer.close();
    m_dictionary_compressor.close();
    m_dictionary_file_writer.close();

    // Delete entries and clear maps
    for (const auto& value_entry_pair : m_value_to_entry) {
        delete value_entry_pair.second;
    }
    m_id_to_entry.clear();
    m_value_to_entry.clear();

    m_is_open = false;
}

template <typename DictionaryIdType, typename EntryType>
const EntryType* DictionaryWriter<DictionaryIdType, EntryType>::get_entry (DictionaryIdType id) const {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    if (m_id_to_entry.count(id) == 0) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    return m_id_to_entry.at(id);
}

template <typename DictionaryIdType, typename EntryType>
void DictionaryWriter<DictionaryIdType, EntryType>::write_uncommitted_entries_to_disk () {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    if (m_uncommitted_entries.empty()) {
        // Nothing to do
        return;
    }

    for (auto entry : m_uncommitted_entries) {
        entry->write_to_file(m_dictionary_compressor);
    }

    // Update header
    auto dictionary_file_writer_pos = m_dictionary_file_writer.get_pos();
    m_dictionary_file_writer.seek_from_begin(0);
    m_dictionary_file_writer.write_numeric_value<uint64_t>(m_id_to_entry.size());
    m_dictionary_file_writer.seek_from_begin(dictionary_file_writer_pos);

    m_segment_index_compressor.flush();
    m_segment_index_file_writer.flush();
    m_dictionary_compressor.flush();
    m_dictionary_file_writer.flush();
    m_uncommitted_entries.clear();
}

template <typename DictionaryIdType, typename EntryType>
void DictionaryWriter<DictionaryIdType, EntryType>::index_segment (segment_id_t segment_id, const std::unordered_set<DictionaryIdType>& ids) {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    m_segment_index_compressor.write_numeric_value(segment_id);

    // NOTE: The IDs in `ids` are not validated to exist in this dictionary since we perform validation when loading the dictionary.
    m_segment_index_compressor.write_numeric_value<uint64_t>(ids.size());
    for (auto id : ids) {
        m_segment_index_compressor.write_numeric_value(id);
    }

    ++m_num_segments_in_index;

    // Update header
    auto segment_index_file_writer_pos = m_segment_index_file_writer.get_pos();
    m_segment_index_file_writer.seek_from_begin(0);
    m_segment_index_file_writer.write_numeric_value<uint64_t>(m_num_segments_in_index);
    m_segment_index_file_writer.seek_from_begin(segment_index_file_writer_pos);
}

#endif // DICTIONARYWRITER_HPP
