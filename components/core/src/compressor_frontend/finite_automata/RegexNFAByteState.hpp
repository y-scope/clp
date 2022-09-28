#ifndef COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_NFA_BYTE_STATE_HPP
#define COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_NFA_BYTE_STATE_HPP

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
        void add_interval (Interval interval, RegexNFAByteState* dest_state);

        /// TODO: is there a better way than copying all the epsilon stuff from base class
        /// TODO: same question for byte stuff in RegexNFAUTF8State.hpp (which is currently missing)
        /// TODO: same questions for DFA stuff
        void add_epsilon_transition (RegexNFAByteState* epsilon_transition) {
            m_epsilon_transitions.push_back(epsilon_transition);
        }

        void clear_epsilon_transitions () {
            m_epsilon_transitions.clear();
        }

        [[nodiscard]] const std::vector<RegexNFAByteState*>& get_epsilon_transitions () const {
            return m_epsilon_transitions;
        }

        void add_byte_transition (uint8_t byte, RegexNFAByteState* dest_state) {
            m_bytes_transitions[byte].push_back(dest_state);
        }

        void clear_byte_transitions (uint8_t byte) {
            m_bytes_transitions[byte].clear();
        }

        [[nodiscard]] const std::vector<RegexNFAByteState*>& get_byte_transitions (uint8_t byte) const {
            return m_bytes_transitions[byte];
        }

    private:
        std::vector<RegexNFAByteState*> m_epsilon_transitions;
        std::vector<RegexNFAByteState*> m_bytes_transitions[cSizeOfByte];
    };
}

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_NFA_BYTE_STATE_HPP
