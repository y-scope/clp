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
        
        struct State {
        public:
            State* next (uint32_t c);

            void add_tag (const int& value) {
                m_tags.push_back(value);
            }

            [[nodiscard]] const std::vector<int>& get_tags () const {
                return m_tags;
            }

            [[nodiscard]] State** get_bytes_transition () {
                return m_bytes_transition;
            };
            
            void add_byte_transition (const uint8_t& byte, State*& byte_transition) {
                m_bytes_transition[byte] = byte_transition;
            }

            void add_tree_transition (const std::pair<uint32_t, uint32_t> key, State*& dest_state) {
                m_tree_transitions.insert(key, dest_state);
            }

            bool is_accepting () {
                return !m_tags.empty();
            }

        private:
            std::vector<int> m_tags;
            State* m_bytes_transition[cSizeOfByte];
            Tree m_tree_transitions;
        };
        
        State* root () { return m_states.at(0).get(); }

        State* new_state (const RegexNFA::StateSet* set);
        
    private:
        std::vector<std::unique_ptr<State>> m_states;
    };
}

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEXDFA_HPP