#ifndef CLP_S_TIMESTAMPDICTIONARYREADER_HPP
#define CLP_S_TIMESTAMPDICTIONARYREADER_HPP

#include <map>
#include <optional>

#include "FileReader.hpp"
#include "search/FilterOperation.hpp"
#include "TimestampEntry.hpp"
#include "TimestampPattern.hpp"
#include "ZstdDecompressor.hpp"

namespace clp_s {
class TimestampDictionaryReader {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructors
    TimestampDictionaryReader() : m_is_open(false) {}

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
    void read_new_entries(bool local = false);

    /**
     * Reads new entries from a *local* timestamp dictionary
     *
     * Local timestamp dictionaries contain only range indices,
     * and have no timestamp pattern mappings
     */
    void read_local_entries();

    /**
     * Gets the string encoding for a given epoch and format ID
     * @param epoch
     * @param format_id
     */
    std::string get_string_encoding(epochtime_t epoch, uint64_t format_id) const;

    /**
     * Gets iterators for the timestamp patterns
     * @return begin and end iterators for the timestamp patterns
     */
    auto pattern_begin() const { return m_patterns.begin(); }

    auto pattern_end() const { return m_patterns.end(); }

    /**
     * Gets iterators for the column to range mappings
     * @return begin and end iterators for the column to range mappings
     */
    auto tokenized_column_to_range_begin() const { return m_tokenized_column_to_range.begin(); }

    auto tokenized_column_to_range_end() const { return m_tokenized_column_to_range.end(); }

    /**
     * @return the tokens for the authoritative timestamp column if it exists, or an
     * empty option if it does not
     */
    std::optional<std::vector<std::string>> get_authoritative_timestamp_column() const;

private:
    typedef std::map<uint64_t, TimestampPattern> id_to_pattern_t;
    typedef std::map<std::string, TimestampEntry> column_to_range_t;
    typedef std::vector<std::pair<std::vector<std::string>, TimestampEntry*>>
            tokenized_column_to_range_t;

    // Variables
    bool m_is_open;
    FileReader m_dictionary_file_reader;
    ZstdDecompressor m_dictionary_decompressor;

    id_to_pattern_t m_patterns;
    column_to_range_t m_column_to_range;
    tokenized_column_to_range_t m_tokenized_column_to_range;
};
}  // namespace clp_s

#endif  // CLP_S_TIMESTAMPDICTIONARYREADER_HPP
