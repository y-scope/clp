#ifndef COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_UTF8_STATE_HPP
#define COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_UTF8_STATE_HPP

#include "RegexDFAByteState.hpp"

// Project headers
#include "UnicodeIntervalTree.hpp"

namespace compressor_frontend::finite_automata {
    class RegexDFAUTF8State : public RegexDFAByteState {
    public:

        typedef UnicodeIntervalTree<RegexDFAUTF8State*> Tree;

        /**
         * Specify which state is transitioned to on an input byte
         * @param byte
         * @param dest_state
         */
        void add_byte_transition (const uint8_t& byte, RegexDFAUTF8State*& dest_state);

        /**
         * Returns the next state the DFA transitions to on input character (byte or utf8)
         * @param character
         * @return State*
         */
        RegexDFAUTF8State* next (uint32_t character);

    private:

        RegexDFAUTF8State* m_bytes_transition[cSizeOfByte];
        Tree m_tree_transitions;
    };
}

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_UTF8_STATE_HPP