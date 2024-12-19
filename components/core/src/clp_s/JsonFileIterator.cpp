#include "JsonFileIterator.hpp"

#include <cctype>
#include <cstring>

#include <spdlog/spdlog.h>

namespace clp_s {
JsonFileIterator::JsonFileIterator(
        clp::ReaderInterface& reader,
        size_t max_document_size,
        size_t buf_size
)
        : m_buf_size(buf_size),
          m_max_document_size(max_document_size),
          m_buf(new char[buf_size + simdjson::SIMDJSON_PADDING]),
          m_reader(reader) {
    read_new_json();
}

JsonFileIterator::~JsonFileIterator() {
    delete[] m_buf;
}

bool JsonFileIterator::read_new_json() {
    m_first_doc_in_buffer = true;
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

        if (clp::ErrorCode::ErrorCode_EndOfFile == file_error) {
            m_eof = true;
        } else if (clp::ErrorCode::ErrorCode_Success != file_error) {
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
        // Only implements != so this is equivalent to "if no JSON is available from the buffer"
        if (false == (m_doc_it != m_stream.end())) {
            // Only allow documents to be up to a certain size so we can conserve memory in
            // exceptional circumstances
            if (m_buf_occupied > m_max_document_size) {
                m_error_code = simdjson::error_code::CAPACITY;
                return false;
            }
            m_truncated_bytes = m_buf_occupied;
        } else {
            return true;
        }
    } while (false == m_eof);
    return true;
}

size_t JsonFileIterator::skip_whitespace_and_get_truncated_bytes() {
    while (m_next_document_position < m_buf_occupied
           && std::isspace(m_buf[m_next_document_position]))
    {
        ++m_next_document_position;
    }
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
                m_first_doc_in_buffer = false;
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

        if (maybe_utf8_edge_case && m_first_doc_in_buffer
            && (m_buf_occupied > m_max_document_size || m_eof))
        {
            // If we hit a UTF-8 error and either we have reached eof or the buffer occupancy is
            // greater than the maximum document size we assume that the UTF-8 error must have been
            // in the middle of the stream. Note: it is possible that the UTF-8 error is at the end
            // of the stream and that this is actualy a truncation error. Unfortunately the only way
            // to check is to parse it ourselves, so we rely on this heuristic for now.
            m_error_code = simdjson::error_code::UTF8_ERROR;
            return false;
        } else if (maybe_utf8_edge_case) {
            m_truncated_bytes = skip_whitespace_and_get_truncated_bytes();
        }
    } while (read_new_json());
    return false;
}

size_t JsonFileIterator::get_num_bytes_consumed() {
    // If there are more documents left in the current buffer account for how much of the
    // buffer has been consumed, otherwise report the total number of bytes read so that we
    // capture trailing whitespace.
    if (m_doc_it != m_stream.end()) {
        return m_bytes_read - (m_buf_occupied - m_next_document_position);
    }
    return m_bytes_read;
}
}  // namespace clp_s
