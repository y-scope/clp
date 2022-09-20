#ifndef COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEXNFA_HPP
#define COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEXNFA_HPP

// C++ standard libraries
#include <algorithm>
#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <vector>

// Project headers
#include "../Constants.hpp"
#include "UnicodeIntervalTree.hpp"

namespace compressor_frontend::finite_automata {
    class RegexDFA;

    class RegexNFA {
    public:
        class State;

        typedef std::vector<State*> StateVec;
        typedef std::set<State*> StateSet;
        typedef UnicodeIntervalTree <StateVec> Tree;

        class State {
        public:
            void add_interval (Tree::Interval interval, State* state);

            void set_accepting (bool value) {
                m_accepting = value;
            }

            [[nodiscard]] const bool& is_accepting () const {
                return m_accepting;
            }

            void set_tag (int value) {
                m_tag = value;
            }

            [[nodiscard]] const int& get_tag () const {
                return m_tag;
            }

            void add_epsilon_transition (State* epsilon_transition) {
                m_epsilon_transitions.push_back(epsilon_transition);
            }

            void clear_epsilon_transitions () {
                m_epsilon_transitions.clear();
            }

            [[nodiscard]] const StateVec& get_epsilon_transitions () const {
                return m_epsilon_transitions;
            }

            void add_byte_transition (uint8_t byte, State* byte_transition) {
                m_bytes_transitions[byte].push_back(byte_transition);
            }

            void clear_byte_transitions (uint8_t byte) {
                m_bytes_transitions[byte].clear();
            }

            [[nodiscard]] const StateVec& get_byte_transitions (uint8_t byte) const {
                return m_bytes_transitions[byte];
            }

            void reset_tree_transitions () {
                m_tree_transitions.reset();
            }

            const Tree& get_tree_transitions () {
                return m_tree_transitions;
            }

        private:
            bool m_accepting;
            int m_tag;
            StateVec m_bytes_transitions[cSizeOfByte];
            Tree m_tree_transitions;
            StateVec m_epsilon_transitions;
        };


        RegexNFA ();

        State* new_state ();

        std::unique_ptr<RegexDFA> to_dfa ();

        void reverse ();

        void add_root_interval (Tree::Interval interval, State* state) {
            m_root->add_interval(interval, state);
        }

        State* m_root;

    private:

        static void epsilon_closure (StateSet* epsilon_closure, State* state);

        std::vector<std::unique_ptr<State>> m_states;
    };
}

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEXNFA_HPP