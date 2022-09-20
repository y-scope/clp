#include "RegexNFA.hpp"

// C++ standard libraries
#include <algorithm>
#include <cassert>
#include <map>
#include <stack>

// Project headers
#include "RegexDFA.hpp"
#include "UnicodeIntervalTree.hpp"

using std::map;
using std::max;
using std::min;
using std::pair;
using std::stack;
using std::unique_ptr;
using std::vector;

namespace compressor_frontend::finite_automata {
    RegexNFA::RegexNFA () {
        m_root = new_state();
    }

    RegexNFA::State* RegexNFA::new_state () {
        unique_ptr<State> ptr(new State());
        State* state = ptr.get();
        m_states.push_back(std::move(ptr));
        return state;
    }

    void RegexNFA::State::add_interval (Tree::Interval interval, State* state) {
        if (interval.first < cSizeOfByte) {
            uint32_t bound = min(interval.second, cSizeOfByte - 1);
            for (uint32_t i = interval.first; i <= bound; i++) {
                m_bytes_transitions[i].push_back(state);
            }
            interval.first = bound + 1;
        }
        if (interval.first > interval.second) {
            return;
        }

        unique_ptr<vector<Tree::Data>> overlaps = m_tree_transitions.pop(interval);
        for (const Tree::Data& data: *overlaps) {
            uint32_t overlap_low = max(data.m_interval.first, interval.first);
            uint32_t overlap_high = min(data.m_interval.second, interval.second);

            StateVec tree_states = data.m_value;
            tree_states.push_back(state);
            m_tree_transitions.insert(Tree::Interval(overlap_low, overlap_high), tree_states);
            if (data.m_interval.first < interval.first) {
                m_tree_transitions.insert(Tree::Interval(data.m_interval.first, interval.first - 1), data.m_value);
            } else if (data.m_interval.first > interval.first) {
                m_tree_transitions.insert(Tree::Interval(interval.first, data.m_interval.first - 1), {state});
            }
            if (data.m_interval.second > interval.second) {
                m_tree_transitions.insert(Tree::Interval(interval.second + 1, data.m_interval.second), data.m_value);
            }
            interval.first = data.m_interval.second + 1;
        }
        if (interval.first != 0 && interval.first <= interval.second) {
            m_tree_transitions.insert(interval, {state});
        }
    }
    
    void RegexNFA::epsilon_closure (StateSet* epsilon_closure, State* state) {
        stack<State*> stack;
        stack.push(state);
        while (!stack.empty()) {
            State* t = stack.top();
            stack.pop();
            if (epsilon_closure->insert(t).second) {
                for (State* const u: t->get_epsilon_transitions()) {
                    stack.push(u);
                }
            }
        }
    }
    
    unique_ptr<RegexDFA> RegexNFA::to_dfa () {
        unique_ptr<RegexDFA> dfa(new RegexDFA);
        map<StateSet, RegexDFA::State*> dfa_states;
        stack<StateSet> unmarked_sets;

        auto create_dfa_state =
                [&dfa, &dfa_states, &unmarked_sets] (const StateSet& set) -> RegexDFA::State* {
                    RegexDFA::State* state = dfa->new_state(&set);
                    dfa_states[set] = state;
                    unmarked_sets.push(set);
                    return state;
                };

        StateSet start_set;
        RegexNFA::epsilon_closure(&start_set, m_root);
        create_dfa_state(start_set);

        while (!unmarked_sets.empty()) {
            StateSet set = unmarked_sets.top();
            unmarked_sets.pop();
            RegexDFA::State* dfa_state = dfa_states.at(set);

            map<uint32_t, StateSet> ascii_transitions_map;
            map<Tree::Interval, StateSet> transitions_map;

            for (State* s0: set) {
                for (uint32_t i = 0; i < cSizeOfByte; i++) {
                    for (State* const s1: s0->get_byte_transitions(i)) {
                        epsilon_closure(&ascii_transitions_map[i], s1);
                    }
                }
                for (const Tree::Data& data: s0->get_tree_transitions().all()) {
                    for (State* const s1: data.m_value) {
                        epsilon_closure(&transitions_map[data.m_interval], s1);
                    }
                }
            }

            auto next_dfa_state =
                    [&dfa_states, &create_dfa_state] (const StateSet& set) -> RegexDFA::State* {
                        RegexDFA::State* state;
                        auto it = dfa_states.find(set);
                        if (it == dfa_states.end()) {
                            state = create_dfa_state(set);
                        } else {
                            state = it->second;
                        }
                        return state;
                    };

            for (const map<uint32_t, StateSet>::value_type& kv: ascii_transitions_map) {
                RegexDFA::State* dest_state = next_dfa_state(kv.second);
                dfa_state->add_byte_transition(kv.first, dest_state);
            }
            for (const map<Tree::Interval, StateSet>::value_type& kv: transitions_map) {
                RegexDFA::State* dest_state = next_dfa_state(kv.second);
                dfa_state->add_tree_transition(kv.first, dest_state);
            }
        }
        return dfa;
    }

    void RegexNFA::reverse () {
        // add new end with all accepting pointing to it
        State* new_end = new_state();
        for (unique_ptr<State>& state_ptr: m_states) {
            if (state_ptr->is_accepting()) {
                state_ptr->add_epsilon_transition(new_end);
                state_ptr->set_accepting(false);
            }
        }
        // move edges
        map<pair<State*, State*>, vector<uint8_t>> byte_edges;
        map<pair<State*, State*>, bool> epsilon_edges;
        for (unique_ptr<State>& src_state_ptr: m_states) {
            src_state_ptr->reset_tree_transitions();
            for (uint32_t byte = 0; byte < cSizeOfByte; byte++) {
                for (State* dest_state_ptr: src_state_ptr->get_byte_transitions(byte)) {
                    byte_edges[pair<State*, State*>(src_state_ptr.get(), dest_state_ptr)].push_back(byte);
                }
                src_state_ptr->clear_byte_transitions(byte);
            }
            for (State* dest_state_ptr: src_state_ptr->get_epsilon_transitions()) {
                epsilon_edges[pair<State*, State*>(src_state_ptr.get(), dest_state_ptr)] = true;
            }
            src_state_ptr->clear_epsilon_transitions();
        }

        // reverse edges
        for (unique_ptr<State>& src_state_ptr: m_states) {
            for (unique_ptr<State>& dest_state_ptr: m_states) {
                pair<State*, State*> key(src_state_ptr.get(), dest_state_ptr.get());
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
        for (State* old_accepting_state: new_end->get_epsilon_transitions()) {
            int tag = old_accepting_state->get_tag();
            stack<State*> unvisited_states;
            std::set<State*> visited_states;
            unvisited_states.push(old_accepting_state);
            while (!unvisited_states.empty()) {
                State* current_state = unvisited_states.top();
                current_state->set_tag(tag);
                unvisited_states.pop();
                visited_states.insert(current_state);
                for(uint32_t byte  = 0; byte < cSizeOfByte; byte++) {
                    StateVec byte_transitions = current_state->get_byte_transitions(byte);
                    for (State* next_state: byte_transitions) {
                        if (visited_states.find(next_state) == visited_states.end()) {
                            unvisited_states.push(next_state);
                        }
                    }
                }
                for (State* next_state: current_state->get_epsilon_transitions()) {
                    if (visited_states.find(next_state) == visited_states.end()) {
                        unvisited_states.push(next_state);
                    }
                }
            }
        }
        for (int32_t i = m_states.size() - 1; i >= 0; i--) {
            unique_ptr<State>& src_state_unique_ptr = m_states[i];
            State* src_state = src_state_unique_ptr.get();
            int tag = src_state->get_tag();
            for(uint32_t byte  = 0; byte < cSizeOfByte; byte++) {
                StateVec byte_transitions = src_state->get_byte_transitions(byte);
                for (int32_t j = byte_transitions.size() - 1; j >= 0; j--) {
                    State*& dest_state = byte_transitions[j];
                    if (dest_state == m_root) {
                        dest_state = new_state();
                        assert(dest_state != nullptr);
                        dest_state->set_tag(tag);
                        dest_state->set_accepting(true);
                    }
                }
            }
            for (State* dest_state: src_state->get_epsilon_transitions()) {
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
}