#ifndef COMPRESSOR_FRONTEND_INPUT_BUFFER_HPP
#define COMPRESSOR_FRONTEND_INPUT_BUFFER_HPP

// Project Headers
#include "../ReaderInterface.hpp"
#include "Buffer.hpp"

namespace compressor_frontend {
    class InputBuffer : public Buffer<char> {
    public:

        /**
         * Resets input buffer
         * @return
         */
        void reset () override;

        /**
         * Checks if reading into the input buffer won't overwrite data not yet used
         * (e.g., data being overwritten is already compressed in the case of compression)
         * @return bool
         */
        bool read_is_safe ();

        /**
         * Reads into the half of the buffer currently available
         * @param reader
         */
        void read (ReaderInterface& reader);

        /**
         * Reads if no unused data will be overwritten
         * @param reader
         */
        void try_read (ReaderInterface& reader) {
            if (read_is_safe()) {
                read(reader);
            }
        }

        /**
         * Swaps to a dynamic buffer (or doubles its size) if needed
         * @return bool
         */
        bool increase_size_and_read (ReaderInterface& reader, size_t& old_storage_size);

        /**
         * Check if at end of file, and return next char (or EOF)
         * @return unsigned char
         */
        unsigned char get_next_character ();

        bool all_data_read () {
            if (m_last_read_first_half) {
                return (m_curr_pos == m_curr_storage_size / 2);
            } else {
                return (m_curr_pos == 0);
            }
        }

        void set_consumed_pos (uint32_t consumed_pos) {
            m_consumed_pos = consumed_pos;
        }

        void set_at_end_of_file (bool at_end_of_file) {
            m_at_end_of_file = at_end_of_file;
        }

        [[nodiscard]] bool at_end_of_file () const {
            return m_at_end_of_file;
        }

    private:
        uint32_t m_bytes_read;
        uint32_t m_consumed_pos;
        bool m_last_read_first_half;
        bool m_finished_reading_file;
        bool m_at_end_of_file;
    };
}

#endif // COMPRESSOR_FRONTEND_INPUT_BUFFER_HPP