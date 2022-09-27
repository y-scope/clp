#ifndef COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_BYTE_HPP
#define COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_BYTE_HPP

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
        RegexDFAState* next (uint32_t character) override;

    private:

        RegexDFAState* m_bytes_transition[cSizeOfByte];
    };

    class RegexDFAByte : public RegexDFA<RegexDFAByteState> {};

}

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_BYTE_HPP