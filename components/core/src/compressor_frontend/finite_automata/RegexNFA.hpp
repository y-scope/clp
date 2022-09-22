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
            /**
             * Add dest_state to m_bytes_transitions if all values in interval are a byte, otherwise add dest_state to m_tree_transitions
             * @param interval
             * @param state
             */
            void add_interval (Tree::Interval interval, State* dest_state);

            /**
             * Set if state is accepting
             * @param accepting
             */
            void set_accepting (bool accepting) {
                m_accepting = accepting;
            }
            
            /**
             * Return if state is accepting
             * @return const bool&
             */
            [[nodiscard]] const bool& is_accepting () const {
                return m_accepting;
            }
            
            /**
             * Set regex rule name associated with the NFA state
             * @param rule_name_id
             */
            void set_tag (int rule_name_id) {
                m_tag = rule_name_id;
            }
            
            /**
             * Get regex rule name associated with the NFA state
             * @return const int&
             */
            [[nodiscard]] const int& get_tag () const {
                return m_tag;
            }

            /**
             * Add epsilon transition to NFA state
             * @param epsilon_transition
             */
            void add_epsilon_transition (State* epsilon_transition) {
                m_epsilon_transitions.push_back(epsilon_transition);
            }
            
            /**
             * Clear epsilon transitions
             */
            void clear_epsilon_transitions () {
                m_epsilon_transitions.clear();
            }
            
            /**
             * Return epsilon transitions 
             * @return const StateVec&
             */
            [[nodiscard]] const StateVec& get_epsilon_transitions () const {
                return m_epsilon_transitions;
            }
            
            /**
             * Add transition on byte to dest_state
             * @param byte
             * @param dest_state
             */
            void add_byte_transition (uint8_t byte, State* dest_state) {
                m_bytes_transitions[byte].push_back(dest_state);
            }
            
            /**
             * Clear state transitions for a specific byte
             * @param byte
             */
            void clear_byte_transitions (uint8_t byte) {
                m_bytes_transitions[byte].clear();
            }
            
            /**
             * Return byte transitions for a specific byte
             * @param byte
             * @return const StateVec& 
             */
            [[nodiscard]] const StateVec& get_byte_transitions (uint8_t byte) const {
                return m_bytes_transitions[byte];
            }

            /**
             * Clear tree transitions
             */
            void reset_tree_transitions () {
                m_tree_transitions.reset();
            }
            
            /**
             * Return tree transitions
             * @return const Tree&
             */
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

        // constructor
        RegexNFA ();

        /**
         * Create a unique_ptr for an NFA state and add it to m_states
         * @return State*
         */
        State* new_state ();
        
        /**
         * Generate a DFA from the NFA
         * @return std::unique_ptr<RegexDFA>
         */
        std::unique_ptr<RegexDFA> to_dfa ();

        /**
         * Reverse the NFA such that it matches on its reverse language 
         */
        void reverse ();
        
        /**
         For m_root, add dest_state to m_bytes_transitions if all values in interval are a byte, otherwise add dest_state to m_tree_transitions
         * @param interval
         * @param state
         */
        void add_root_interval (Tree::Interval interval, State* dest_state) {
            m_root->add_interval(interval, dest_state);
        }

        State* m_root;

    private:

        /**
         * Add to epsilon_closure all the states reachable from the specified state by an epsilon transition
         * @param epsilon_closure
         * @param state
         */
        static void epsilon_closure (StateSet* epsilon_closure, State* state);

        std::vector<std::unique_ptr<State>> m_states;
    };
}

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEXNFA_HPP