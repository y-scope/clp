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
#include "UnicodeIntervalTree.hpp"

namespace compressor_frontend::finite_automata {
    enum class RegexNFAStateType {
        Byte,
        UTF8
    };

    template <RegexNFAStateType stateType>
    class RegexNFAState {
    public:

        using Tree = UnicodeIntervalTree<RegexNFAState<stateType>*>;

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

        void set_epsilon_transitions (std::vector<RegexNFAState<stateType>*>& epsilon_transitions) {
            m_epsilon_transitions = epsilon_transitions;
        }

        void add_epsilon_transition (RegexNFAState<stateType>* epsilon_transition) {
            m_epsilon_transitions.push_back(epsilon_transition);
        }

        void clear_epsilon_transitions () {
            m_epsilon_transitions.clear();
        }

        [[nodiscard]] const std::vector<RegexNFAState<stateType>*>& get_epsilon_transitions () const {
            return m_epsilon_transitions;
        }

        void set_byte_transitions (uint8_t byte, std::vector<RegexNFAState<stateType>*>& byte_transitions) {
            m_bytes_transitions[byte] = byte_transitions;
        }

        void add_byte_transition (uint8_t byte, RegexNFAState<stateType>* dest_state) {
            m_bytes_transitions[byte].push_back(dest_state);
        }

        void clear_byte_transitions (uint8_t byte) {
            m_bytes_transitions[byte].clear();
        }

        [[nodiscard]] const std::vector<RegexNFAState<stateType>*>& get_byte_transitions (uint8_t byte) const {
            return m_bytes_transitions[byte];
        }

        void reset_tree_transitions () {
            m_tree_transitions.reset();
        }

        const Tree& get_tree_transitions () {
            return m_tree_transitions;
        }

        /**
         Add dest_state to m_bytes_transitions if all values in interval are a byte, otherwise add dest_state to m_tree_transitions
         * @param interval
         * @param dest_state
         */
        void add_interval (Interval interval, RegexNFAState<stateType>* dest_state);

    private:
        bool m_accepting;
        int m_tag;
        std::vector<RegexNFAState<stateType>*> m_epsilon_transitions;
        std::vector<RegexNFAState<stateType>*> m_bytes_transitions[cSizeOfByte];

        // NOTE: We don't need m_tree_transitions for the `stateType == RegexDFAStateType::Byte` case,
        // so we use an empty class (`std::tuple<>`) in that case.
        std::conditional_t<stateType == RegexNFAStateType::UTF8, Tree, std::tuple<>> m_tree_transitions;


    };

    using RegexNFAByteState = RegexNFAState<RegexNFAStateType::Byte>;
    using RegexNFAUTF8State = RegexNFAState<RegexNFAStateType::UTF8>;

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