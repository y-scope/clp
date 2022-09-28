#ifndef COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_NFA_UTF8_STATE_HPP
#define COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_NFA_UTF8_STATE_HPP

// C++ standard libraries
#include <set>
#include <vector>

// Project headers
#include "RegexNFAByteState.hpp"
#include "UnicodeIntervalTree.hpp"

namespace compressor_frontend::finite_automata {

    class RegexNFAUTF8State : public RegexNFAByteState {
    public:
        typedef UnicodeIntervalTree<std::vector<RegexNFAUTF8State*>> Tree;

        /**
         Add dest_state to m_bytes_transitions if all values in interval are a byte, otherwise add dest_state to m_tree_transitions
         * @param interval
         * @param dest_state
         */
        void add_interval (Interval interval, RegexNFAUTF8State* dest_state);

        void reset_tree_transitions () {
            m_tree_transitions.reset();
        }

        const Tree& get_tree_transitions () {
            return m_tree_transitions;
        }

    private:
        Tree m_tree_transitions;
    };

}

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_NFA_UTF8_STATE_HPP
