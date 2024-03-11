#include "JsonFileIterator.hpp"

#include <cctype>
#include <cstring>

#include <spdlog/spdlog.h>

namespace clp_s {
JsonFileIterator::JsonFileIterator(std::string const& file_name, size_t buf_size)
        : m_buf_size(buf_size),
          m_buf(new char[buf_size + simdjson::SIMDJSON_PADDING]) {
    try {
        m_reader.open(file_name);
    } catch (FileReader::OperationFailed& e) {
        SPDLOG_ERROR("Failed to open {} for reading - {}", file_name, e.what());
        return;
    }

    read_new_json();
}

JsonFileIterator::~JsonFileIterator() {
    delete[] m_buf;
    if (m_reader.is_open()) {
        m_reader.close();
    }
}

bool JsonFileIterator::read_new_json() {
    do {
        if (m_truncated_bytes == m_buf_size) {
            // double buffer size to attempt to capture long json object
            size_t new_buf_size = m_buf_size * 2;
            char* new_buf = new char[new_buf_size + simdjson::SIMDJSON_PADDING];
            memcpy(new_buf, m_buf, m_buf_size);
            delete[] m_buf;
            m_buf = new_buf;
            m_buf_size = new_buf_size;
        } else if (m_truncated_bytes > 0) {
            // move bytes to start of buffer
            memmove(m_buf, m_buf + (m_buf_occupied - m_truncated_bytes), m_truncated_bytes);
            m_buf_occupied = m_truncated_bytes;
        } else {
            m_buf_occupied = 0;
        }

        m_truncated_bytes = 0;
        m_next_document_position = 0;

        size_t size_read = 0;
        auto file_error
                = m_reader.try_read(m_buf + m_buf_occupied, m_buf_size - m_buf_occupied, size_read);
        m_buf_occupied += size_read;
        m_bytes_read += size_read;

        if (ErrorCodeEndOfFile == file_error) {
            m_eof = true;
        } else if (ErrorCodeSuccess != file_error) {
            m_error_code = simdjson::error_code::IO_ERROR;
            return false;
        }

        auto error = m_parser.iterate_many(
                                     m_buf,
                                     /* length of data */ m_buf_occupied,
                                     /* batch size of data to parse*/ m_buf_occupied
        )
                             .get(m_stream);

        if (error) {
            m_error_code = error;
            return false;
        }

        m_doc_it = m_stream.begin();
        // only implements != so this is equivalent to "if no JSON is available from the buffer"
        if (false == (m_doc_it != m_stream.end())) {
            m_truncated_bytes = m_buf_occupied;
        } else {
            return true;
        }
    } while (false == m_eof);
    return true;
}

size_t JsonFileIterator::skip_whitespace_and_get_truncated_bytes() {
    for (;
         m_next_document_position < m_buf_occupied && std::isspace(m_buf[m_next_document_position]);
         ++m_next_document_position)
    {}
    return m_buf_occupied - m_next_document_position;
}

bool JsonFileIterator::get_json(simdjson::ondemand::document_stream::iterator& it) {
    if (false == m_first_read) {
        ++m_doc_it;
    } else {
        m_first_read = false;
    }

    do {
        bool maybe_utf8_edge_case{false};
        if (m_doc_it != m_stream.end()) {
            if (simdjson::error_code::SUCCESS == m_doc_it.error()) {
                it = m_doc_it;

                // There is a bug in simdjson where truncated bytes isn't set correctly when a UTF8
                // character is cut off at the end of the buffer. To work around this limitation we
                // always keep track of the start of the next document ourselves. The bytes after
                // the last valid document and before the end of the buffer are naturally the
                // truncated bytes for the current buffer.
                m_next_document_position = m_doc_it.current_index() + m_doc_it.source().size();
                return true;
            } else if (m_doc_it.error() == simdjson::error_code::UTF8_ERROR) {
                maybe_utf8_edge_case = true;
            } else {
                m_error_code = m_doc_it.error();
                return false;
            }
        } else {
            m_truncated_bytes = skip_whitespace_and_get_truncated_bytes();
            // If we have reached the end of the file and the end of the document stream there are
            // no more documents to iterate over
            if (m_eof) {
                return false;
            }
        }

        if (maybe_utf8_edge_case) {
            // Check if this is the UTF-8 edge case by advancing the iterator and checking if this
            // is really the end of the stream.
            ++m_doc_it;
            if (m_doc_it != m_stream.end()) {
                // Regular UTF-8 error in the middle of the document stream.
                m_error_code = simdjson::error_code::UTF8_ERROR;
                return false;
            } else {
                m_truncated_bytes = skip_whitespace_and_get_truncated_bytes();
            }

            if (m_eof) {
                // UTF-8 error at end of stream. Treat this case as document truncation.
                return false;
            }
        }
    } while (read_new_json());
    return false;
}
}  // namespace clp_s
