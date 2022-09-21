#ifndef COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEXDFA_HPP
#define COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEXDFA_HPP

// C++ standard libraries
#include <algorithm>
#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <vector>

// Project headers
#include "../Constants.hpp"
#include "RegexNFA.hpp"
#include "UnicodeIntervalTree.hpp"

namespace compressor_frontend::finite_automata {

    class RegexDFA {
    public:
        class State;

        typedef UnicodeIntervalTree<State*> Tree;

        class State {
        public:
            /**
             * Returns the next state the DFA transitions to on input character (byte or utf8)
             * @param character
             * @return State*
             */
            State* next (uint32_t character);
            
            /**
             * Adds to m_tags an id indicating a regex rule name that is matched in this state
             * @param rule_name
             */
            void add_tag (const int& rule_name_id) {
                m_tags.push_back(rule_name_id);
            }
            
            /**
             * Returns ids indicating the names of regex rules matched in this state
             * @return const std::vector<int>&
             */
            [[nodiscard]] const std::vector<int>& get_tags () const {
                return m_tags;
            }
            
            /**
             * Specify which state is transitioned to on an input byte  
             * @param byte
             * @param dest_state
             */            
            void add_byte_transition (const uint8_t& byte, State*& dest_state) {
                m_bytes_transition[byte] = dest_state;
            }
            
            /**
             * Specify which state is transitioned to on an input utf8
             * @param key
             * @param dest_state
             */
            void add_tree_transition (const std::pair<uint32_t, uint32_t> key, State*& dest_state) {
                m_tree_transitions.insert(key, dest_state);
            }
            
            /**
             * Returns if state is accepting (matches at least one regex rule)  
             * @return bool
             */
            bool is_accepting () {
                return !m_tags.empty();
            }

        private:
            std::vector<int> m_tags;
            State* m_bytes_transition[cSizeOfByte];
            Tree m_tree_transitions;
        };

        /**
         * Creates a new DFA state based on a set of NFA states and adds it to m_states
         * @param set
         * @return State*
         */  
        State* new_state (const RegexNFA::StateSet* set);

        /**
         * Returns root of DFA  
         * @return State*
         */
        State* get_root () { 
            return m_states.at(0).get(); 
        }
        
    private:
        std::vector<std::unique_ptr<State>> m_states;
    };
}

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEXDFA_HPP