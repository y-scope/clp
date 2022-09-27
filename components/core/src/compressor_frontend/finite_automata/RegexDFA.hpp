#ifndef COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_HPP
#define COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_HPP

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

namespace compressor_frontend::finite_automata {
    class RegexDFAState {
    public:

        // Destructor
        virtual ~RegexDFAState() = 0;

        /**
         * Returns the next state the DFA transitions to on input character (byte or utf8)
         * @param character
         * @return State*
         */
        virtual RegexDFAState* next (uint32_t character) = 0;

        void add_tag (const int& rule_name_id) {
            m_tags.push_back(rule_name_id);
        }

        [[nodiscard]] const std::vector<int>& get_tags () const {
            return m_tags;
        }

        bool is_accepting () {
            return !m_tags.empty();
        }

    private:
        std::vector<int> m_tags;
    };

    template <typename DFAStateType>
    class RegexDFA {
    public:

        /**
         * Creates a new DFA state based on a set of NFA states and adds it to m_states
         * @param set
         * @return DFAStateType*
         */
        template <typename NFAStateType>
        DFAStateType* new_state (const std::set<NFAStateType>* set);

        DFAStateType* get_root () {
            return m_states.at(0).get();
        }

    private:
        std::vector<std::unique_ptr<DFAStateType>> m_states;
    };
}

#include "RegexDFA.tpp"

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_HPP