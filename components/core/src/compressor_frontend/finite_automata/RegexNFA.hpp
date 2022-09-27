#ifndef COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_NFA_HPP
#define COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_NFA_HPP

// C++ standard libraries
#include <algorithm>
#include <cstdint>
#include <memory>
#include <set>
#include <stack>
#include <utility>
#include <vector>

// Project headers
#include "../Constants.hpp"

namespace compressor_frontend::finite_automata {
    class RegexNFAState {
    public:
        typedef std::vector<RegexNFAState*> StateVec;

        /**
         * Return epsilon_closure over m_epsilon_transitions
         * @return
         */
        std::set<RegexNFAState*> epsilon_closure () {
            std::set<RegexNFAState*> closure_set;
            std::stack<RegexNFAState*> stack;
            stack.push(this);
            while (!stack.empty()) {
                RegexNFAState* t = stack.top();
                stack.pop();
                if (closure_set.insert(t).second) {
                    for (RegexNFAState* const u: t->get_epsilon_transitions()) {
                        stack.push(u);
                    }
                }
            }
            return closure_set;
        }

        void set_accepting (bool accepting) {
            m_accepting = accepting;
        }

        [[nodiscard]] const bool& is_accepting () const {
            return m_accepting;
        }

        void set_tag (int rule_name_id) {
            m_tag = rule_name_id;
        }

        [[nodiscard]] const int& get_tag () const {
            return m_tag;
        }

        void add_epsilon_transition (RegexNFAState* epsilon_transition) {
            m_epsilon_transitions.push_back(epsilon_transition);
        }

        void clear_epsilon_transitions () {
            m_epsilon_transitions.clear();
        }

        [[nodiscard]] const StateVec& get_epsilon_transitions () const {
            return m_epsilon_transitions;
        }

        virtual const StateVec& get_byte_transitions (uint8_t byte) const = 0;

    private:
        bool m_accepting;
        int m_tag;
        StateVec m_epsilon_transitions;
    };

    template <typename NFAStateType>
    class RegexNFA {
    public:
        typedef std::vector<NFAStateType*> StateVec;

        // constructor
        RegexNFA ();

        /**
         * Create a unique_ptr for an NFA state and add it to m_states
         * @return NFAStateType*
         */
        NFAStateType* new_state ();

        /**
         * Reverse the NFA such that it matches on its reverse language 
         */
        void reverse ();

        /**
         For m_root, add dest_state to m_bytes_transitions if all values in interval are a byte, otherwise add dest_state to m_tree_transitions
         * @param interval
         * @param dest_state
         */
        void add_root_interval (Interval interval, NFAStateType* dest_state) {
            m_root->add_interval(interval, dest_state);
        }

        NFAStateType* m_root;

    private:
        std::vector<std::unique_ptr<NFAStateType>> m_states;
    };
}

#include "RegexNFA.tpp"

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_NFA_HPP