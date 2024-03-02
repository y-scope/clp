#include "JsonFileIterator.hpp"

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
        // only implements != so this is equivalent to
        // if no json available from buffer and we haven't hit eof
        // then retry reading the json with a larger buffer up to eof
        if (false == (m_doc_it != m_stream.end()) && false == m_eof) {
            m_truncated_bytes = m_buf_occupied;
        } else {
            return true;
        }
    } while (false == m_eof);
    return true;
}

bool JsonFileIterator::get_json(simdjson::ondemand::document_stream::iterator& it) {
    if (false == m_first_read) {
        ++m_doc_it;
    } else {
        m_first_read = false;
    }

    if (m_doc_it != m_stream.end()) {
        if (simdjson::error_code::SUCCESS == m_doc_it.error()) {
            it = m_doc_it;

            // there is a bug in simdjson where when invalid UTF8 is encountered at the end of the
            // stream truncated bytes isn't set correctly. To work around this limitation we keep
            // track of the start of the next document. The bytes after the last valid document are
            // naturally the truncated bytes for the stream.
            m_next_document_position = m_doc_it.current_index() + m_doc_it.source().size();
            return true;
        } else if (m_doc_it.error() == simdjson::error_code::UTF8_ERROR) {
            m_truncated_bytes = m_buf_occupied - m_next_document_position;
        } else {
            m_error_code = m_doc_it.error();
            return false;
        }
    } else if (m_eof) {
        // if we iterate to the end of the stream without error after hitting eof it should be safe
        // to ask the stream iterator for number of truncated bytes
        m_truncated_bytes = m_stream.truncated_bytes();
        return false;
    } else {
        m_truncated_bytes = m_stream.truncated_bytes();
    }

    if (m_truncated_bytes > 0) {
        ++m_doc_it;
        // UTF8 error wasn't at the end of the stream -- terminate
        if (m_doc_it != m_stream.end()) {
            m_error_code = simdjson::error_code::UTF8_ERROR;
            return false;
        }
    }

    if (false == read_new_json()) {
        return false;
    }

    if (m_doc_it != m_stream.end()) {
        if (simdjson::error_code::SUCCESS == m_doc_it.error()) {
            it = m_doc_it;
            return true;
        }
    }

    return false;
}
}  // namespace clp_s
