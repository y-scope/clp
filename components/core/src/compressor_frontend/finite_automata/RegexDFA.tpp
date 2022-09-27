#ifndef COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_TPP
#define COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_TPP

#include "RegexDFA.hpp"

using std::unique_ptr;

namespace compressor_frontend::finite_automata {

    template <typename DFAStateType>
    template <typename NFAStateType>
    DFAStateType* RegexDFA<DFAStateType>::new_state (const std::set<NFAStateType>* set) {
        unique_ptr<DFAStateType> ptr(new DFAStateType());
        m_states.push_back(std::move(ptr));

        DFAStateType* state = m_states.back().get();
        for (const RegexNFAState* s: *set) {
            if (s->is_accepting()) {
                state->add_tag(s->get_tag());
            }
        }
        return state;
    }
}

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_TPP