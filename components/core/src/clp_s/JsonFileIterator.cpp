#include "JsonFileIterator.hpp"

#include <cctype>
#include <cstring>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "Utils.hpp"

namespace clp_s {
JsonFileIterator::JsonFileIterator(
        clp::ReaderInterface& reader,
        size_t max_document_size,
        bool sanitize_invalid_json,
        std::string path,
        size_t buf_size
)
        : m_buf_size(buf_size),
          m_max_document_size(max_document_size),
          m_buf(new char[buf_size + simdjson::SIMDJSON_PADDING]),
          m_reader(reader),
          m_path(std::move(path)),
          m_sanitize_invalid_json(sanitize_invalid_json) {
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

        // If sanitization is enabled and we encounter errors that can be fixed by sanitization,
        // sanitize the buffer and retry parsing
        if (m_sanitize_invalid_json) {
            // Handle invalid UTF-8 sequences by replacing with U+FFFD
            if (simdjson::error_code::UTF8_ERROR == error) {
                // Return value intentionally ignored - in read_new_json we always retry after
                // sanitization regardless of whether changes were made
                static_cast<void>(sanitize_and_log_utf8());
                error = m_parser.iterate_many(m_buf, m_buf_occupied, m_buf_occupied).get(m_stream);
            }

            // Handle unescaped control characters by escaping them to \u00XX format
            if (simdjson::error_code::UNESCAPED_CHARS == error) {
                // Return value intentionally ignored - in read_new_json we always retry after
                // sanitization regardless of whether changes were made
                static_cast<void>(sanitize_and_log_json());
                error = m_parser.iterate_many(m_buf, m_buf_occupied, m_buf_occupied).get(m_stream);
            }
        }

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

bool JsonFileIterator::sanitize_and_log_utf8() {
    size_t const old_buf_occupied = m_buf_occupied;
    auto const result = StringUtils::sanitize_utf8_buffer(
            m_buf,
            m_buf_size,
            m_buf_occupied,
            simdjson::SIMDJSON_PADDING
    );
    m_buf_occupied = result.new_buf_occupied;
    m_sanitization_bytes_added += m_buf_occupied - old_buf_occupied;

    if (result.sanitized_char_counts.empty()) {
        return false;
    }

    size_t total_replaced = 0;
    for (auto const& [ch, count] : result.sanitized_char_counts) {
        total_replaced += count;
    }
    SPDLOG_WARN(
            "Replaced {} invalid UTF-8 sequence(s) with U+FFFD{}. Buffer size changed by {} bytes "
            "({} -> {}).",
            total_replaced,
            m_path.empty() ? "" : fmt::format(" in file '{}'", m_path),
            static_cast<int64_t>(m_buf_occupied) - static_cast<int64_t>(old_buf_occupied),
            old_buf_occupied,
            m_buf_occupied
    );
    return true;
}

bool JsonFileIterator::sanitize_and_log_json() {
    size_t const old_buf_occupied = m_buf_occupied;
    // Note: sanitize_json_buffer may reallocate m_buf and will update m_buf_size by reference if
    // reallocation is needed. This keeps m_buf_size in sync with the actual allocated buffer size.
    auto const result = StringUtils::sanitize_json_buffer(
            m_buf,
            m_buf_size,
            m_buf_occupied,
            simdjson::SIMDJSON_PADDING
    );
    m_buf_occupied = result.new_buf_occupied;
    m_sanitization_bytes_added += m_buf_occupied - old_buf_occupied;

    if (result.sanitized_char_counts.empty()) {
        return false;
    }

    size_t total_sanitized = 0;
    std::string char_details;
    for (auto const& [ch, count] : result.sanitized_char_counts) {
        if (false == char_details.empty()) {
            char_details += ", ";
        }
        char_details += fmt::format("0x{:02x} ({})", static_cast<unsigned char>(ch), count);
        total_sanitized += count;
    }
    SPDLOG_WARN(
            "Escaped {} control character(s) in JSON{}: [{}]. Buffer expanded by {} bytes "
            "({} -> {}).",
            total_sanitized,
            m_path.empty() ? "" : fmt::format(" in file '{}'", m_path),
            char_details,
            m_buf_occupied - old_buf_occupied,
            old_buf_occupied,
            m_buf_occupied
    );
    return true;
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
            } else if (m_sanitize_invalid_json
                       && m_doc_it.error() == simdjson::error_code::UNESCAPED_CHARS)
            {
                // Unescaped control characters detected during document iteration. Sanitize the
                // buffer and re-setup the document stream to restart from the beginning.
                if (false == sanitize_and_log_json()) {
                    // Sanitization made no changes - report the original error to avoid infinite
                    // loop
                    m_error_code = m_doc_it.error();
                    return false;
                }

                // Re-setup the document stream and restart iteration
                auto error = m_parser.iterate_many(m_buf, m_buf_occupied, m_buf_occupied)
                                     .get(m_stream);
                if (error) {
                    m_error_code = error;
                    return false;
                }
                m_doc_it = m_stream.begin();
                m_first_doc_in_buffer = true;
                m_next_document_position = 0;
                continue;
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
            // of the stream and that this is actually a truncation error. Unfortunately the only
            // way to check is to parse it ourselves, so we rely on this heuristic for now.
            if (m_sanitize_invalid_json) {
                // Sanitize invalid UTF-8 sequences and retry
                if (false == sanitize_and_log_utf8()) {
                    // Sanitization made no changes - report the original error to avoid infinite
                    // loop
                    m_error_code = simdjson::error_code::UTF8_ERROR;
                    return false;
                }

                // Re-setup the document stream and restart iteration
                auto error = m_parser.iterate_many(m_buf, m_buf_occupied, m_buf_occupied)
                                     .get(m_stream);
                if (error) {
                    m_error_code = error;
                    return false;
                }
                m_doc_it = m_stream.begin();
                m_first_doc_in_buffer = true;
                m_next_document_position = 0;
                continue;
            }
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
    // capture trailing whitespace. Include bytes added by sanitization since the sanitized
    // content is what gets compressed.
    if (m_doc_it != m_stream.end()) {
        return m_bytes_read + m_sanitization_bytes_added
               - (m_buf_occupied - m_next_document_position);
    }
    return m_bytes_read + m_sanitization_bytes_added;
}
}  // namespace clp_s
