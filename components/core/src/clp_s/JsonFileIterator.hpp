#ifndef CLP_S_JSONFILEITERATOR_HPP
#define CLP_S_JSONFILEITERATOR_HPP

#include <simdjson.h>

#include "FileReader.hpp"

namespace clp_s {
class JsonFileIterator {
public:
    /**
     * An iterator over a file containing json objects. JSON is parsed
     * using simdjson::parse_many. This allows simdjson to efficiently find
     * delimeters between JSON objects, and if enabled parse JSON ahead of time
     * in another thread while the JSON is being iterated over.
     *
     * The buffer grows automatically if there are JSON objects larger than the buffer size.
     * The buffer is padded to be SIMDJSON_PADDING bytes larger than the specified size.

     * @param file_name the file containing JSON
     * @param buf_size the initial buffer size
     */
    explicit JsonFileIterator(
            std::string const& file_name,
            size_t buf_size = 1024 * 1024 /*1MB default*/
    );
    ~JsonFileIterator();

    /**
     * Reads the next JSON document and returns it in the it argument
     * @param it an iterator to the JSON object that gets returned
     * @return true if the iterator is valid, false otherwise
     */
    bool get_json(simdjson::ondemand::document_stream::iterator& it);

    /**
     * Checks if the file is open
     * @return true if the file opened successfully
     */
    bool is_open() { return m_reader.is_open(); }

    /**
     * @return number of truncated bytes after json documents
     */
    size_t truncated_bytes() {
        if (m_stream.size_in_bytes() != 0) {
            return m_stream.truncated_bytes();
        }
        return 0;
    }

    /**
     * @return total number of bytes read from the file
     */
    [[nodiscard]] size_t get_num_bytes_read() { return m_bytes_read; }

private:
    /**
     * Reads new JSON into the buffer and initializes iterators into the data.
     * If the buffer is not large enough to contain the JSON its size is doubled.
     * @param truncated_bytes length of incomplete JSON at end of buffer in bytes
     */
    void read_new_json(size_t truncated_bytes);

    size_t reverse_search_newline_truncated_bytes(size_t start);

    size_t m_bytes_read;
    size_t m_buf_size;
    size_t m_buf_occupied;
    char* m_buf;
    FileReader m_reader;
    simdjson::ondemand::parser m_parser;
    simdjson::ondemand::document_stream m_stream;
    bool m_eof;
    bool m_first_read;
    simdjson::ondemand::document_stream::iterator m_doc_it;
};
}  // namespace clp_s

#endif  // CLP_S_JSONFILEITERATOR_HPP
