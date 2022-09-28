#ifndef COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_NFA_TPP
#define COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_NFA_TPP

#include "RegexNFA.hpp"

// C++ standard libraries
#include <algorithm>
#include <cassert>
#include <map>
#include <stack>

// Project headers
#include "../Constants.hpp"
#include "UnicodeIntervalTree.hpp"

using std::map;
using std::max;
using std::min;
using std::pair;
using std::stack;
using std::unique_ptr;
using std::vector;

namespace compressor_frontend::finite_automata {
    template <typename NFAStateType>
    void RegexNFA<NFAStateType>::reverse () {
        // add new end with all accepting pointing to it
        NFAStateType* new_end = new_state();
        for (unique_ptr<NFAStateType>& state_ptr: m_states) {
            if (state_ptr->is_accepting()) {
                state_ptr->add_epsilon_transition(new_end);
                state_ptr->set_accepting(false);
            }
        }
        // move edges from NFA to maps
        map<pair<NFAStateType*, NFAStateType*>, vector<uint8_t>> byte_edges;
        map<pair<NFAStateType*, NFAStateType*>, bool> epsilon_edges;
        for (unique_ptr<NFAStateType>& src_state_ptr: m_states) {
            // TODO: handle utf8 case
            for (uint32_t byte = 0; byte < cSizeOfByte; byte++) {
                for (NFAStateType* dest_state_ptr: src_state_ptr->get_byte_transitions(byte)) {
                    byte_edges[pair<NFAStateType*, NFAStateType*>(src_state_ptr.get(), dest_state_ptr)].push_back(byte);
                }
                src_state_ptr->clear_byte_transitions(byte);
            }
            for (NFAStateType* dest_state_ptr: src_state_ptr->get_epsilon_transitions()) {
                epsilon_edges[pair<NFAStateType*, NFAStateType*>(src_state_ptr.get(), dest_state_ptr)] = true;
            }
            src_state_ptr->clear_epsilon_transitions();
        }

        // insert edges from maps back into NFA, but in the reverse direction
        for (unique_ptr<NFAStateType>& src_state_ptr: m_states) {
            for (unique_ptr<NFAStateType>& dest_state_ptr: m_states) {
                pair<NFAStateType*, NFAStateType*> key(src_state_ptr.get(), dest_state_ptr.get());
                auto byte_it = byte_edges.find(key);
                if (byte_it != byte_edges.end()) {
                    for (uint8_t byte: byte_it->second) {
                        dest_state_ptr->add_byte_transition(byte, src_state_ptr.get());
                    }
                }
                auto epsilon_it = epsilon_edges.find(key);
                if (epsilon_it != epsilon_edges.end()) {
                    dest_state_ptr->add_epsilon_transition(src_state_ptr.get());
                }
            }
        }

        // propagate tag from old accepting m_states
        for (NFAStateType* old_accepting_state: new_end->get_epsilon_transitions()) {
            int tag = old_accepting_state->get_tag();
            stack<NFAStateType*> unvisited_states;
            std::set<NFAStateType*> visited_states;
            unvisited_states.push(old_accepting_state);
            while (!unvisited_states.empty()) {
                NFAStateType* current_state = unvisited_states.top();
                current_state->set_tag(tag);
                unvisited_states.pop();
                visited_states.insert(current_state);
                for(uint32_t byte  = 0; byte < cSizeOfByte; byte++) {
                    std::vector<NFAStateType*> byte_transitions = current_state->get_byte_transitions(byte);
                    for (NFAStateType* next_state: byte_transitions) {
                        if (visited_states.find(next_state) == visited_states.end()) {
                            unvisited_states.push(next_state);
                        }
                    }
                }
                for (NFAStateType* next_state: current_state->get_epsilon_transitions()) {
                    if (visited_states.find(next_state) == visited_states.end()) {
                        unvisited_states.push(next_state);
                    }
                }
            }
        }
        for (int32_t i = m_states.size() - 1; i >= 0; i--) {
            unique_ptr<NFAStateType>& src_state_unique_ptr = m_states[i];
            NFAStateType* src_state = src_state_unique_ptr.get();
            int tag = src_state->get_tag();
            for(uint32_t byte  = 0; byte < cSizeOfByte; byte++) {
                std::vector<NFAStateType*> byte_transitions = src_state->get_byte_transitions(byte);
                for (int32_t j = byte_transitions.size() - 1; j >= 0; j--) {
                    NFAStateType*& dest_state = byte_transitions[j];
                    if (dest_state == m_root) {
                        dest_state = new_state();
                        assert(dest_state != nullptr);
                        dest_state->set_tag(tag);
                        dest_state->set_accepting(true);
                    }
                }
            }
            for (NFAStateType* dest_state: src_state->get_epsilon_transitions()) {
                if (dest_state == m_root) {
                    dest_state = new_state();
                    dest_state->set_tag(src_state->get_tag());
                    dest_state->set_accepting(true);
                }
            }
        }

        for (uint32_t i = 0; i < m_states.size(); i++) {
            if (m_states[i].get() == m_root) {
                m_states.erase(m_states.begin() + i);
                break;
            }
        }
        // start from the end
        m_root = new_end;

    }

    template <typename NFAStateType>
    RegexNFA<NFAStateType>::RegexNFA () {
        m_root = new_state();
    }

    template <typename NFAStateType>
    NFAStateType* RegexNFA<NFAStateType>::new_state () {
        unique_ptr<NFAStateType> ptr = std::make_unique<NFAStateType>();
        NFAStateType* state = ptr.get();
        m_states.push_back(std::move(ptr));
        return state;
    }
}

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_NFA_TPP