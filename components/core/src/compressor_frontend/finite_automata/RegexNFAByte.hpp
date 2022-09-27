#ifndef COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_NFA_BYTE_HPP
#define COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_NFA_BYTE_HPP

// C++ standard libraries
#include <set>
#include <vector>

// Project headers
#include "RegexNFA.hpp"

namespace compressor_frontend::finite_automata {

    class RegexNFAByteState : public RegexNFAState {
    public:

        /**
         Add dest_state to m_bytes_transitions if all values in interval are a byte, otherwise add dest_state to m_tree_transitions
         * @param interval
         * @param dest_state
         */
        void add_interval (Interval interval, RegexNFAState* dest_state);

        void add_byte_transition (uint8_t byte, RegexNFAState* dest_state) {
            m_bytes_transitions[byte].push_back(dest_state);
        }

        void clear_byte_transitions (uint8_t byte) {
            m_bytes_transitions[byte].clear();
        }

        [[nodiscard]] const StateVec& get_byte_transitions (uint8_t byte) const {
            return m_bytes_transitions[byte];
        }

    private:
        StateVec m_bytes_transitions[cSizeOfByte];
    };
}

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_NFA_BYTE_HPP
