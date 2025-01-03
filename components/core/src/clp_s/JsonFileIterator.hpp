#ifndef CLP_S_JSONFILEITERATOR_HPP
#define CLP_S_JSONFILEITERATOR_HPP

#include <simdjson.h>

#include "../clp/ReaderInterface.hpp"

namespace clp_s {
class JsonFileIterator {
public:
    /**
     * An iterator over an input stream containing json objects. JSON is parsed
     * using simdjson::parse_many. This allows simdjson to efficiently find
     * delimeters between JSON objects, and if enabled parse JSON ahead of time
     * in another thread while the JSON is being iterated over.
     *
     * The buffer grows automatically if there are JSON objects larger than the buffer size.
     * The buffer is padded to be SIMDJSON_PADDING bytes larger than the specified size.

     * @param reader the input stream containing JSON
     * @param max_document_size the maximum allowed size of a single document
     * @param buf_size the initial buffer size
     */
    explicit JsonFileIterator(
            clp::ReaderInterface& reader,
            size_t max_document_size,
            size_t buf_size = 1024 * 1024 /*1MB default*/
    );
    ~JsonFileIterator();

    /**
     * Reads the next JSON document and returns it in the it argument
     * @param it an iterator to the JSON object that gets returned
     * @return true if the iterator is valid, false otherwise
     */
    [[nodiscard]] bool get_json(simdjson::ondemand::document_stream::iterator& it);

    /**
     * @return number of truncated bytes after json documents
     */
    [[nodiscard]] size_t truncated_bytes() const { return m_truncated_bytes; }

    /**
     * @return total number of bytes read from the file
     */
    [[nodiscard]] size_t get_num_bytes_read() const { return m_bytes_read; }

    /**
     * Note: this method can not be const because checking if a simdjson iterator is at the end
     * of a document stream is non-const.
     *
     * @return total number of bytes consumed from the file via get_json
     */
    [[nodiscard]] size_t get_num_bytes_consumed();

    /**
     * @return the last error code encountered when iterating over the json file
     */
    [[nodiscard]] simdjson::error_code get_error() const { return m_error_code; }

private:
    /**
     * Reads new JSON into the buffer and initializes iterators into the data.
     * If the buffer is not large enough to contain the JSON its size is doubled.
     * @return true if the new JSON was read successfully, false otherwise
     */
    bool read_new_json();

    /**
     * Advance the m_next_document_position pointer past any whitespace then return the number of
     * truncated bytes in the buffer.
     * @return the number of truncated bytes in the buffer
     */
    [[nodiscard]] size_t skip_whitespace_and_get_truncated_bytes();

    size_t m_truncated_bytes{0};
    size_t m_next_document_position{0};
    size_t m_bytes_read{0};
    size_t m_buf_size{0};
    size_t m_buf_occupied{0};
    size_t m_max_document_size{0};
    char* m_buf{nullptr};
    clp::ReaderInterface& m_reader;
    simdjson::ondemand::parser m_parser;
    simdjson::ondemand::document_stream m_stream;
    bool m_eof{false};
    bool m_first_read{true};
    bool m_first_doc_in_buffer{false};
    simdjson::ondemand::document_stream::iterator m_doc_it;
    simdjson::error_code m_error_code{simdjson::error_code::SUCCESS};
};
}  // namespace clp_s

#endif  // CLP_S_JSONFILEITERATOR_HPP
