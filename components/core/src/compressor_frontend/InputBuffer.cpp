// C++ libraries
#include <memory.h>
#include <string>

// spdlog
#include <spdlog/spdlog.h>

// Project Headers
#include "InputBuffer.hpp"

using std::string;
using std::to_string;

namespace compressor_frontend {

    void InputBuffer::reset () {
        m_at_end_of_file = false;
        m_finished_reading_file = false;
        m_consumed_pos = 0;
        m_bytes_read = 0;
        m_last_read_first_half = false;
        Buffer::reset();
    }

    bool InputBuffer::read_is_safe () {
        if (m_finished_reading_file) {
            return false;
        }
        // If the next message starts at 0, the previous character is at m_curr_storage_size - 1
        if (m_consumed_pos == -1) {
            m_consumed_pos = m_curr_storage_size - 1;
        }
        // Check that the last log message ends in the half of the buffer that was last read.
        // This means the other half of the buffer has already been fully used.
        if ((!m_last_read_first_half && m_consumed_pos > m_curr_storage_size / 2) ||
            (m_last_read_first_half && m_consumed_pos < m_curr_storage_size / 2 &&
             m_consumed_pos > 0)) {
            return true;
        }
        return false;
    }

    bool InputBuffer::increase_size_and_read (ReaderInterface& reader, size_t& old_storage_size) {
        old_storage_size = m_curr_storage_size;
        bool flipped_static_buffer = false;
        // Handle super long line for completeness, but efficiency doesn't matter
        if (m_active_storage == m_static_storage) {
            SPDLOG_WARN("Long line detected changing to dynamic input buffer and"
                        " increasing size to {}.", m_curr_storage_size * 2);
        } else {
            SPDLOG_WARN("Long line detected increasing dynamic input buffer size to {}.",
                        m_curr_storage_size * 2);
        }
        m_dynamic_storages.emplace_back((char*)malloc(2 * m_curr_storage_size * sizeof(char)));
        if (m_dynamic_storages.back() == nullptr) {
            SPDLOG_ERROR("Failed to allocate input buffer of size {}.", m_curr_storage_size);
            string err = "Lexer failed to find a match after checking entire buffer";
            throw std::runtime_error(err);
        }
        if (m_last_read_first_half == false) {
            // Buffer in correct order
            memcpy(m_dynamic_storages.back(), m_active_storage,
                   m_curr_storage_size * sizeof(char));
        } else {
            // Buffer out of order, so it needs to be flipped when copying
            memcpy(m_dynamic_storages.back(),
                   m_active_storage + m_curr_storage_size * sizeof(char) / 2,
                   m_curr_storage_size * sizeof(char) / 2);
            memcpy(m_dynamic_storages.back() + m_curr_storage_size * sizeof(char) / 2,
                   m_active_storage, m_curr_storage_size * sizeof(char) / 2);
            flipped_static_buffer = true;
        }
        m_curr_storage_size *= 2;
        m_active_storage = m_dynamic_storages.back();
        m_bytes_read = m_curr_storage_size / 2;
        m_curr_pos = m_curr_storage_size / 2;
        read(reader);
        return flipped_static_buffer;
    }

    unsigned char InputBuffer::get_next_character () {
        if (m_finished_reading_file && m_curr_pos == m_bytes_read) {
            m_at_end_of_file = true;
            return utf8::cCharEOF;
        }
        unsigned char character = m_active_storage[m_curr_pos];
        m_curr_pos++;
        if (m_curr_pos == m_curr_storage_size) {
            m_curr_pos = 0;
        }
        return character;
    }

    void InputBuffer::read (ReaderInterface& reader) {
        size_t bytes_read;
        // read into the correct half of the buffer
        uint32_t read_offset = 0;
        if (m_last_read_first_half) {
            read_offset = m_curr_storage_size / 2;
        }
        reader.read(m_active_storage + read_offset, m_curr_storage_size / 2, bytes_read);
        m_last_read_first_half = !m_last_read_first_half;
        if (bytes_read < m_curr_storage_size / 2) {
            m_finished_reading_file = true;
        }
        m_bytes_read += bytes_read;
        if (m_bytes_read > m_curr_storage_size) {
            m_bytes_read -= m_curr_storage_size;
        }
    }
}
