// Code from CLP

#ifndef CLP_S_DICTIONARYWRITER_HPP
#define CLP_S_DICTIONARYWRITER_HPP

#include "DictionaryEntry.hpp"

namespace clp_s {
template <typename DictionaryIdType, typename EntryType>
class DictionaryWriter {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructors
    DictionaryWriter() : m_is_open(false) {}

    ~DictionaryWriter() = default;

    // Methods
    /**
     * Opens dictionary for writing
     * @param dictionary_path
     * @param compression_level
     * @param max_id
     */
    void open(std::string const& dictionary_path, int compression_level, DictionaryIdType max_id);

    /**
     * Closes the dictionary
     * @return the compressed size of the dictionary in bytes
     */
    [[nodiscard]] size_t close();

    /**
     * Writes the dictionary's header and flushes unwritten content to disk
     */

    void write_header_and_flush_to_disk();

    /**
     * @return The size (in-memory) of the data contained in the dictionary
     */
    size_t get_data_size() const { return m_data_size; }

protected:
    // Types
    using value_to_id_t = std::unordered_map<std::string, DictionaryIdType>;

    // Variables
    bool m_is_open;

    // Variables related to on-disk storage
    FileWriter m_dictionary_file_writer;
    ZstdCompressor m_dictionary_compressor;

    value_to_id_t m_value_to_id;
    uint64_t m_next_id{};
    uint64_t m_max_id{};

    // Size (in-memory) of the data contained in the dictionary
    size_t m_data_size{};
};

class VariableDictionaryWriter : public DictionaryWriter<uint64_t, VariableDictionaryEntry> {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    /**
     * Adds the given variable to the dictionary if it doesn't exist.
     * @param value
     * @param id ID of the variable matching the given entry
     */
    bool add_entry(std::string const& value, uint64_t& id);
};

class LogTypeDictionaryWriter : public DictionaryWriter<uint64_t, LogTypeDictionaryEntry> {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    /**
     * Adds the given entry to the dictionary if it doesn't exist
     * @param logtype_entry
     * @param logtype_id ID of the logtype matching the given entry
     */
    bool add_entry(LogTypeDictionaryEntry& logtype_entry, uint64_t& logtype_id);
};

template <typename DictionaryIdType, typename EntryType>
void DictionaryWriter<DictionaryIdType, EntryType>::open(
        std::string const& dictionary_path,
        int compression_level,
        DictionaryIdType max_id
) {
    if (m_is_open) {
        throw OperationFailed(ErrorCodeNotReady, __FILENAME__, __LINE__);
    }

    m_dictionary_file_writer.open(dictionary_path, FileWriter::OpenMode::CreateForWriting);
    // Write header
    m_dictionary_file_writer.write_numeric_value<uint64_t>(0);
    // Open compressor
    m_dictionary_compressor.open(m_dictionary_file_writer, compression_level);

    m_next_id = 0;
    m_max_id = max_id;

    m_data_size = 0;
    m_is_open = true;
}

template <typename DictionaryIdType, typename EntryType>
size_t DictionaryWriter<DictionaryIdType, EntryType>::close() {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCodeNotInit, __FILENAME__, __LINE__);
    }

    write_header_and_flush_to_disk();
    m_dictionary_compressor.close();
    size_t compressed_size = m_dictionary_file_writer.get_pos();
    m_dictionary_file_writer.close();

    m_value_to_id.clear();

    m_is_open = false;
    return compressed_size;
}

template <typename DictionaryIdType, typename EntryType>
void DictionaryWriter<DictionaryIdType, EntryType>::write_header_and_flush_to_disk() {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCodeNotInit, __FILENAME__, __LINE__);
    }

    // Update header
    auto dictionary_file_writer_pos = m_dictionary_file_writer.get_pos();
    m_dictionary_file_writer.seek_from_begin(0);
    m_dictionary_file_writer.write_numeric_value<uint64_t>(m_value_to_id.size());
    m_dictionary_file_writer.seek_from_begin(dictionary_file_writer_pos);

    m_dictionary_compressor.flush();
    m_dictionary_file_writer.flush();
}
}  // namespace clp_s

#endif  // CLP_S_DICTIONARYWRITER_HPP
