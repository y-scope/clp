#ifndef COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_UTF8_HPP
#define COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_UTF8_HPP

#include "RegexDFAByte.hpp"

// Project headers
#include "UnicodeIntervalTree.hpp"

namespace compressor_frontend::finite_automata {
    class RegexDFAUTF8State : public RegexDFAByteState {
    public:

        typedef UnicodeIntervalTree<RegexDFAUTF8State*> Tree;

        /**
         * Returns the next state the DFA transitions to on input character (byte or utf8)
         * @param character
         * @return State*
         */
        RegexDFAState* next (uint32_t character) override;

    private:

        Tree m_tree_transitions;
    };

    class RegexDFAUTF8 : public RegexDFA<RegexDFAUTF8State> {};

}

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_UTF8_HPP