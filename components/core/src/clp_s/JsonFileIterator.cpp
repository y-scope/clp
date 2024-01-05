#include "JsonFileIterator.hpp"

#include <cstring>

namespace clp_s {
JsonFileIterator::JsonFileIterator(std::string const& file_name, size_t buf_size) {
    m_buf = nullptr;

    try {
        m_reader.open(file_name);
    } catch (FileReader::OperationFailed& e) {
        return;
    }

    m_eof = false;
    m_buf_size = buf_size;
    m_buf = new char[buf_size + simdjson::SIMDJSON_PADDING];
    m_buf_occupied = 0;
    m_first_read = true;
    m_bytes_read = 0;

    read_new_json(/*truncated_bytes=*/0);
}

JsonFileIterator::~JsonFileIterator() {
    delete[] m_buf;
    if (false == m_reader.is_open()) {
        m_reader.close();
    }
}

void JsonFileIterator::read_new_json(size_t truncated_bytes) {
    if (truncated_bytes == m_buf_size) {
        // double buffer size to attempt to capture long json object
        size_t new_buf_size = m_buf_size * 2;
        char* new_buf = new char[new_buf_size + simdjson::SIMDJSON_PADDING];
        memcpy(new_buf, m_buf, m_buf_size);
        delete[] m_buf;
        m_buf = new_buf;
        m_buf_size = new_buf_size;
    } else if (truncated_bytes > 0) {
        // move bytes to start of buffer
        memmove(m_buf, m_buf + (m_buf_occupied - truncated_bytes), truncated_bytes);
        m_buf_occupied = truncated_bytes;
    } else {
        m_buf_occupied = 0;
    }

    size_t size_read = 0;
    auto error = m_reader.try_read(m_buf + m_buf_occupied, m_buf_size - m_buf_occupied, size_read);
    m_buf_occupied += size_read;
    m_bytes_read += size_read;

    if (error != ErrorCodeSuccess) {
        m_eof = true;
    }

    m_parser.iterate_many(
                    m_buf,
                    /* length of data */ m_buf_occupied,
                    /* batch size of data to parse*/ m_buf_occupied
    )
            .get(m_stream);

    m_doc_it = m_stream.begin();
    // only implements != so this is equivalent to
    // if no json available from buffer and we haven't hit eof
    // then retry reading the json with a larger buffer up to eof
    if (false == (m_doc_it != m_stream.end()) && false == m_eof) {
        read_new_json(m_stream.truncated_bytes());
    }
}

bool JsonFileIterator::get_json(simdjson::ondemand::document_stream::iterator& it) {
    if (false == m_first_read) {
        ++m_doc_it;
    } else {
        m_first_read = false;
    }

    size_t patch_truncated_bytes = 0;
    if (m_doc_it != m_stream.end()) {
        if (m_doc_it.error() == simdjson::error_code::SUCCESS) {
            it = m_doc_it;
            return true;
        } else if (m_doc_it.error() == simdjson::error_code::UTF8_ERROR) {
            patch_truncated_bytes
                    = reverse_search_newline_truncated_bytes(m_doc_it.current_index());
        }
    } else if (m_eof) {
        return false;
    }

    // there is a bug in simdjson where when invalid utf8 is encountered
    // at the end of the stream truncated bytes isn't set correctly.
    // Work around this limitation by manually searching for the start
    // of the erroring document and stetting truncated bytes appropriately
    if (patch_truncated_bytes == 0) {
        read_new_json(m_stream.truncated_bytes());
    } else {
        read_new_json(patch_truncated_bytes);
    }

    if (m_doc_it != m_stream.end()) {
        if (m_doc_it.error() == simdjson::error_code::SUCCESS) {
            it = m_doc_it;
            return true;
        }
    }

    return false;
}

size_t JsonFileIterator::reverse_search_newline_truncated_bytes(size_t start) {
    if (m_buf_occupied == 0) {
        return 0;
    }

    if (start > m_buf_occupied) {
        start = m_buf_occupied - 1;
    }

    while (start > 0 && m_buf[start] != '\n') {
        --start;
    }

    return m_buf_occupied - start - 1;
}
}  // namespace clp_s
