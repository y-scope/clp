#ifndef DICTIONARYREADER_HPP
#define DICTIONARYREADER_HPP

// C++ standard libraries
#include <string>
#include <vector>

// Boost libraries
#include <boost/algorithm/string.hpp>

// Project headers
#include "dictionary_utils.hpp"
#include "DictionaryEntry.hpp"
#include "FileReader.hpp"
#include "streaming_compression/passthrough/Decompressor.hpp"
#include "streaming_compression/zstd/Decompressor.hpp"
#include "string_utils.hpp"
#include "Utils.hpp"

/**
 * Template class for reading dictionaries from disk and performing operations on them
 * @tparam DictionaryIdType
 * @tparam EntryType
 */
template <typename DictionaryIdType, typename EntryType>
class DictionaryReader {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "DictionaryReader operation failed";
        }
    };

    // Constructors
    DictionaryReader () : m_is_open(false), m_num_segments_read_from_index(0) {
        static_assert(std::is_base_of<DictionaryEntry<DictionaryIdType>, EntryType>::value, "EntryType must be DictionaryEntry or a derivative.");
    }

    // Methods
    /**
     * Opens dictionary for reading
     * @param dictionary_path
     * @param segment_index_path
     */
    void open (const std::string& dictionary_path, const std::string& segment_index_path);
    /**
     * Closes the dictionary
     */
    void close ();

    /**
     * Reads any new entries from disk
     */
    void read_new_entries ();

    /**
     * Gets the dictionary's entries
     * @return All dictionary entries
     */
    const std::vector<EntryType>& get_entries () const { return m_entries; }

    /**
     * Gets the entry with the given ID
     * @param id
     * @return The entry with the given ID
     */
    const EntryType& get_entry (DictionaryIdType id) const;

    /**
     * Gets the value of the entry with the specified ID
     * @param id
     * @return Value of the entry with the specified ID
     */
    const std::string& get_value (DictionaryIdType id) const;
    /**
     * Gets the entry exactly matching the given search string
     * @param search_string
     * @param ignore_case
     * @return nullptr if an exact match is not found, the entry otherwise
     */
    const EntryType* get_entry_matching_value (const std::string& search_string, bool ignore_case) const;
    /**
     * Gets the entries that match a given wildcard string
     * @param wildcard_string
     * @param ignore_case
     * @param entries Set in which to store found entries
     */
    void get_entries_matching_wildcard_string (const std::string& wildcard_string, bool ignore_case, std::unordered_set<const EntryType*>& entries) const;

protected:
    // Methods
    /**
     * Reads a segment's worth of IDs from the segment index
     */
    void read_segment_ids ();

    // Variables
    bool m_is_open;
    FileReader m_dictionary_file_reader;
    FileReader m_segment_index_file_reader;
#if USE_PASSTHROUGH_COMPRESSION
    streaming_compression::passthrough::Decompressor m_dictionary_decompressor;
    streaming_compression::passthrough::Decompressor m_segment_index_decompressor;
#elif USE_ZSTD_COMPRESSION
    streaming_compression::zstd::Decompressor m_dictionary_decompressor;
    streaming_compression::zstd::Decompressor m_segment_index_decompressor;
#else
    static_assert(false, "Unsupported compression mode.");
#endif
    size_t m_num_segments_read_from_index;
    std::vector<EntryType> m_entries;
};

template <typename DictionaryIdType, typename EntryType>
void DictionaryReader<DictionaryIdType, EntryType>::open (const std::string& dictionary_path, const std::string& segment_index_path) {
    if (m_is_open) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }

    constexpr size_t cDecompressorFileReadBufferCapacity = 64 * 1024; // 64 KB

    open_dictionary_for_reading(dictionary_path, segment_index_path, cDecompressorFileReadBufferCapacity, m_dictionary_file_reader, m_dictionary_decompressor,
                                m_segment_index_file_reader, m_segment_index_decompressor);

    m_is_open = true;
}

template <typename DictionaryIdType, typename EntryType>
void DictionaryReader<DictionaryIdType, EntryType>::close () {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    m_segment_index_decompressor.close();
    m_segment_index_file_reader.close();
    m_dictionary_decompressor.close();
    m_dictionary_file_reader.close();

    m_num_segments_read_from_index = 0;
    m_entries.clear();

    m_is_open = false;
}

template <typename DictionaryIdType, typename EntryType>
void DictionaryReader<DictionaryIdType, EntryType>::read_new_entries () {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    // Read dictionary header
    auto num_dictionary_entries = read_dictionary_header(m_dictionary_file_reader);

    // Validate dictionary header
    if (num_dictionary_entries < m_entries.size()) {
        throw OperationFailed(ErrorCode_Corrupt, __FILENAME__, __LINE__);
    }

    // Read new dictionary entries
    if (num_dictionary_entries > m_entries.size()) {
        auto prev_num_dictionary_entries = m_entries.size();
        m_entries.resize(num_dictionary_entries);

        for (size_t i = prev_num_dictionary_entries; i < num_dictionary_entries; ++i) {
            auto& entry = m_entries[i];

            entry.read_from_file(m_dictionary_decompressor);
        }
    }

    // Read segment index header
    auto num_segments = read_segment_index_header(m_segment_index_file_reader);

    // Validate segment index header
    if (num_segments < m_num_segments_read_from_index) {
        throw OperationFailed(ErrorCode_Corrupt, __FILENAME__, __LINE__);
    }

    // Read new segments from index
    if (num_segments > m_num_segments_read_from_index) {
        for (size_t i = m_num_segments_read_from_index; i < num_segments; ++i) {
            read_segment_ids();
        }
    }
}

template <typename DictionaryIdType, typename EntryType>
const EntryType& DictionaryReader<DictionaryIdType, EntryType>::get_entry (DictionaryIdType id) const {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }
    if (id >= m_entries.size()) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }

    return m_entries[id];
}

template <typename DictionaryIdType, typename EntryType>
const std::string& DictionaryReader<DictionaryIdType, EntryType>::get_value (DictionaryIdType id) const {
    if (id >= m_entries.size()) {
        throw OperationFailed(ErrorCode_Corrupt, __FILENAME__, __LINE__);
    }
    return m_entries[id].get_value();
}

template <typename DictionaryIdType, typename EntryType>
const EntryType* DictionaryReader<DictionaryIdType, EntryType>::get_entry_matching_value (const std::string& search_string, bool ignore_case) const {
    if (false == ignore_case) {
        for (const auto& entry : m_entries) {
            if (entry.get_value() == search_string) {
                return &entry;
            }
        }
    } else {
        const auto& search_string_uppercase = boost::algorithm::to_upper_copy(search_string);
        for (const auto& entry : m_entries) {
            if (boost::algorithm::to_upper_copy(entry.get_value()) == search_string_uppercase) {
                return &entry;
            }
        }
    }

    return nullptr;
}

template <typename DictionaryIdType, typename EntryType>
void DictionaryReader<DictionaryIdType, EntryType>::get_entries_matching_wildcard_string (const std::string& wildcard_string, bool ignore_case,
                                                                                          std::unordered_set<const EntryType*>& entries) const
{
    for (const auto& entry : m_entries) {
        if (wildcard_match_unsafe(entry.get_value(), wildcard_string, false == ignore_case)) {
            entries.insert(&entry);
        }
    }
}

template <typename DictionaryIdType, typename EntryType>
void DictionaryReader<DictionaryIdType, EntryType>::read_segment_ids () {
    segment_id_t segment_id;
    m_segment_index_decompressor.read_numeric_value(segment_id, false);

    uint64_t num_ids;
    m_segment_index_decompressor.read_numeric_value(num_ids, false);
    for (uint64_t i = 0; i < num_ids; ++i) {
        DictionaryIdType id;
        m_segment_index_decompressor.read_numeric_value(id, false);
        if (id >= m_entries.size()) {
            throw OperationFailed(ErrorCode_Corrupt, __FILENAME__, __LINE__);
        }

        m_entries[id].add_segment_containing_entry(segment_id);
    }
}

#endif // DICTIONARYREADER_HPP
