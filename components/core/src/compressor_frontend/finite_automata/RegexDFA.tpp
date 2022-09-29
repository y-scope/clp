#ifndef COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_TPP
#define COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_TPP

#include "RegexDFA.hpp"

namespace compressor_frontend::finite_automata {

    template<RegexDFAStateType stateType>
    RegexDFAState<stateType>* RegexDFAState<stateType>::next (uint32_t character) {
        if constexpr (RegexDFAStateType::Byte == stateType) {
           return m_bytes_transition[character];
        } else {
            if (character < cSizeOfByte) {
              return m_bytes_transition[character];
            }
            unique_ptr<vector<typename Tree::Data>> result = m_tree_transitions.find(Interval(character, character));
            assert(result->size() <= 1);
            if (!result->empty()) {
                return result->front().m_value;
            }
            return nullptr;
        }
    }

    template <typename DFAStateType>
    template <typename NFAStateType>
    DFAStateType* RegexDFA<DFAStateType>::new_state (const std::set<NFAStateType*>& set) {
        std::unique_ptr<DFAStateType> ptr = std::make_unique<DFAStateType>();
        m_states.push_back(std::move(ptr));

        DFAStateType* state = m_states.back().get();
        for (const NFAStateType* s: set) {
            if (s->is_accepting()) {
                state->add_tag(s->get_tag());
            }
        }
        return state;
    }
}

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_DFA_TPP