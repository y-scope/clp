#ifndef COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_BYTE_STATE_HPP
#define COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_BYTE_STATE_HPP

#include "RegexDFA.hpp"

namespace compressor_frontend::finite_automata {
    class RegexDFAByteState : public RegexDFAState {
    public:
        /**
         * Specify which state is transitioned to on an input byte
         * @param byte
         * @param dest_state
         */
        void add_byte_transition (const uint8_t& byte, RegexDFAByteState*& dest_state);

        /**
         * Returns the next state the DFA transitions to on input character (byte or utf8)
         * @param character
         * @return State*
         */
        RegexDFAByteState* next (uint32_t character);

    private:

        RegexDFAByteState* m_bytes_transition[cSizeOfByte];
    };
}

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_BYTE_STATE_HPP