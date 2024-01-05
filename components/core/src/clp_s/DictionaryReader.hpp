// Code from CLP

#ifndef CLP_S_DICTIONARYREADER_HPP
#define CLP_S_DICTIONARYREADER_HPP

#include <unordered_set>

#include <boost/algorithm/string/case_conv.hpp>

#include "DictionaryEntry.hpp"
#include "Utils.hpp"

namespace clp_s {
template <typename DictionaryIdType, typename EntryType>
class DictionaryReader {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructors
    DictionaryReader() : m_is_open(false) {}

    // Methods
    /**
     * Opens dictionary for reading
     * @param dictionary_path
     */
    void open(std::string const& dictionary_path);

    /**
     * Closes the dictionary
     */
    void close();

    /**
     * Reads any new entries from disk
     */
    void read_new_entries(bool lazy = false);

    /**
     * @return All dictionary entries
     */
    std::vector<EntryType> const& get_entries() const { return m_entries; }

    /**
     * @param id
     * @return The entry with the given ID
     */
    EntryType& get_entry(DictionaryIdType id);

    /**
     * @param id
     * @return Value of the entry with the specified ID
     */
    std::string const& get_value(DictionaryIdType id) const;

    /**
     * Gets the entry exactly matching the given search string
     * @param search_string
     * @param ignore_case
     * @return nullptr if an exact match is not found, the entry otherwise
     */
    EntryType const*
    get_entry_matching_value(std::string const& search_string, bool ignore_case) const;

    /**
     * Gets the entries that match a given wildcard string
     * @param wildcard_string
     * @param ignore_case
     * @param entries Set in which to store found entries
     */
    void get_entries_matching_wildcard_string(
            std::string const& wildcard_string,
            bool ignore_case,
            std::unordered_set<EntryType const*>& entries
    ) const;

protected:
    bool m_is_open;
    FileReader m_dictionary_file_reader;
    ZstdDecompressor m_dictionary_decompressor;
    std::vector<EntryType> m_entries;
};

class VariableDictionaryReader : public DictionaryReader<uint64_t, VariableDictionaryEntry> {};

class LogTypeDictionaryReader : public DictionaryReader<uint64_t, LogTypeDictionaryEntry> {};

template <typename DictionaryIdType, typename EntryType>
void DictionaryReader<DictionaryIdType, EntryType>::open(std::string const& dictionary_path) {
    if (m_is_open) {
        throw OperationFailed(ErrorCodeNotReady, __FILENAME__, __LINE__);
    }

    constexpr size_t cDecompressorFileReadBufferCapacity = 64 * 1024;  // 64 KB

    m_dictionary_file_reader.open(dictionary_path);
    // Skip header
    m_dictionary_file_reader.seek_from_begin(sizeof(uint64_t));
    // Open decompressor
    m_dictionary_decompressor.open(m_dictionary_file_reader, cDecompressorFileReadBufferCapacity);

    m_is_open = true;
}

template <typename DictionaryIdType, typename EntryType>
void DictionaryReader<DictionaryIdType, EntryType>::close() {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCodeNotReady, __FILENAME__, __LINE__);
    }

    m_dictionary_decompressor.close();
    m_dictionary_file_reader.close();

    m_is_open = false;
}

template <typename DictionaryIdType, typename EntryType>
void DictionaryReader<DictionaryIdType, EntryType>::read_new_entries(bool lazy) {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCodeNotInit, __FILENAME__, __LINE__);
    }

    auto dictionary_file_reader_pos = m_dictionary_file_reader.get_pos();
    m_dictionary_file_reader.seek_from_begin(0);
    uint64_t num_dictionary_entries;
    m_dictionary_file_reader.read_numeric_value(num_dictionary_entries, false);
    m_dictionary_file_reader.seek_from_begin(dictionary_file_reader_pos);

    // Validate dictionary header
    if (num_dictionary_entries < m_entries.size()) {
        throw OperationFailed(ErrorCodeCorrupt, __FILENAME__, __LINE__);
    }

    // Read new dictionary entries
    if (num_dictionary_entries > m_entries.size()) {
        auto prev_num_dictionary_entries = m_entries.size();
        m_entries.resize(num_dictionary_entries);

        for (size_t i = prev_num_dictionary_entries; i < num_dictionary_entries; ++i) {
            auto& entry = m_entries[i];
            entry.read_from_file(m_dictionary_decompressor, i, lazy);
        }
    }
}

template <typename DictionaryIdType, typename EntryType>
EntryType& DictionaryReader<DictionaryIdType, EntryType>::get_entry(DictionaryIdType id) {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCodeNotInit, __FILENAME__, __LINE__);
    }
    if (id >= m_entries.size()) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }

    return m_entries[id];
}

template <typename DictionaryIdType, typename EntryType>
std::string const& DictionaryReader<DictionaryIdType, EntryType>::get_value(DictionaryIdType id
) const {
    if (id >= m_entries.size()) {
        throw OperationFailed(ErrorCodeCorrupt, __FILENAME__, __LINE__);
    }
    return m_entries[id].get_value();
}

template <typename DictionaryIdType, typename EntryType>
EntryType const* DictionaryReader<DictionaryIdType, EntryType>::get_entry_matching_value(
        std::string const& search_string,
        bool ignore_case
) const {
    if (false == ignore_case) {
        for (auto const& entry : m_entries) {
            if (entry.get_value() == search_string) {
                return &entry;
            }
        }
    } else {
        auto const& search_string_uppercase = boost::algorithm::to_upper_copy(search_string);
        for (auto const& entry : m_entries) {
            if (boost::algorithm::to_upper_copy(entry.get_value()) == search_string_uppercase) {
                return &entry;
            }
        }
    }

    return nullptr;
}

template <typename DictionaryIdType, typename EntryType>
void DictionaryReader<DictionaryIdType, EntryType>::get_entries_matching_wildcard_string(
        std::string const& wildcard_string,
        bool ignore_case,
        std::unordered_set<EntryType const*>& entries
) const {
    for (auto const& entry : m_entries) {
        if (StringUtils::wildcard_match_unsafe(entry.get_value(), wildcard_string, !ignore_case)) {
            entries.insert(&entry);
        }
    }
}
}  // namespace clp_s

#endif  // CLP_S_DICTIONARYREADER_HPP
