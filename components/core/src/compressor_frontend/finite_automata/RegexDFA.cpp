#include "RegexDFA.hpp"

// C++ standard libraries
#include <cassert>
#include <map>
#include <stack>

// Project headers
#include "RegexNFA.hpp"
#include "UnicodeIntervalTree.hpp"

using std::unique_ptr;
using std::vector;

namespace compressor_frontend::finite_automata {

    RegexDFA::State* RegexDFA::new_state (const RegexNFA::StateSet* set) {
        unique_ptr<State> ptr(new State());
        m_states.push_back(std::move(ptr));
        
        State* state = m_states.back().get();
        for (const RegexNFA::State* s: *set) {
            if (s->is_accepting()) {
                state->add_tag(s->get_tag());
            }
        }
        return state;
    }

    /// TODO: make this next different for schema lexing vs log lexing
    RegexDFA::State* RegexDFA::State::next (uint32_t character) {
        if (character < cSizeOfByte) {
            return m_bytes_transition[character];
        }
        unique_ptr<vector<Tree::Data>> result = m_tree_transitions.find(Tree::Interval(character, character));
        assert(result->size() <= 1);
        if (!result->empty()) {
            return result->front().m_value;
        }
        return nullptr;
    }
    
}
