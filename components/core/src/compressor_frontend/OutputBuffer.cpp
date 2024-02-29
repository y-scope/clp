#include "OutputBuffer.hpp"

// C++ standard libraries
#include <string>

// spdlog
#include <spdlog/spdlog.h>

using std::string;

namespace compressor_frontend {
    void OutputBuffer::increment_pos () {
        m_curr_pos++;
        if (m_curr_pos == m_curr_storage_size) {
            if (m_active_storage == m_static_storage) {
                SPDLOG_WARN(
                        "Very long log detected: changing to a dynamic output buffer and "
                        "increasing size to {}. Expect increased latency.",
                        m_curr_storage_size * 2);
            } else {
                SPDLOG_WARN("Very long log detected: increasing dynamic output buffer size to {}.",
                            m_curr_storage_size * 2);
            }
            m_dynamic_storages.emplace_back(
                    (Token*)malloc(2 * m_curr_storage_size * sizeof(Token)));
            if (m_dynamic_storages.back() == nullptr) {
                SPDLOG_ERROR("Failed to allocate output buffer of size {}.", m_curr_storage_size);
                /// TODO: update exception when they're  properly
                /// (e.g., "failed_to_compress_log_continue_to_next")
                throw std::runtime_error(
                        "Lexer failed to find a match after checking entire buffer");
            }
            memcpy(m_dynamic_storages.back(), m_active_storage,
                   m_curr_storage_size * sizeof(Token));
            m_active_storage = m_dynamic_storages.back();
            m_curr_storage_size *= 2;
        }
    }

    void OutputBuffer::reset () {
        m_has_timestamp = false;
        m_has_delimiters = false;
        Buffer::reset();
    }
}
