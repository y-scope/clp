#ifndef COMPRESSOR_FRONTEND_OUTPUT_BUFFER_HPP
#define COMPRESSOR_FRONTEND_OUTPUT_BUFFER_HPP

// Project Headers
#include "Buffer.hpp"
#include "Token.hpp"

/**
 * A buffer containing the tokenized output of the parser.
 * The active buffer contains all the tokens from the current log message.
 * The first token contains the timestamp (if there is no timestamp the first token is invalid).
 * For performance (runtime latency) it defaults to a static buffer and when more tokens are needed
 * to be stored than the current capacity it switches to a dynamic buffer.
 * Each time the capacity is exceeded a new dynamic buffer is added to the list of dynamic buffers.
 */
namespace compressor_frontend {

    class OutputBuffer : public Buffer<Token> {
    public:

        /**
         * Increment buffer pos, swaps to a dynamic buffer (or doubles its size) if needed
         */
        void increment_pos ();

        /**
         * Resets output buffer
         * @return
         */
        void reset () override;

        void set_has_timestamp (bool has_timestamp) {
            m_has_timestamp = has_timestamp;
        }

        [[nodiscard]] bool has_timestamp () const {
            return m_has_timestamp;
        }

        void set_has_delimiters (bool has_delimiters) {
            m_has_delimiters = has_delimiters;
        }

        [[nodiscard]] bool has_delimiters () const {
            return m_has_delimiters;
        }

        void set_token (uint32_t pos, Token& value) {
            m_active_storage[pos] = value;
        }

        void set_curr_token (Token& value) {
            m_active_storage[m_curr_pos] = value;
        }

        [[nodiscard]] const Token& get_curr_token () const {
            return m_active_storage[m_curr_pos];
        }

    private:
        bool m_has_timestamp = false;
        bool m_has_delimiters = false;
    };
}

#endif // COMPRESSOR_FRONTEND_OUTPUT_BUFFER_HPP