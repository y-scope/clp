#include "RegexDFAUTF8State.hpp"

// C++ standard libraries
#include <cassert>
#include "../Constants.hpp"

using std::unique_ptr;
using std::vector;

namespace compressor_frontend::finite_automata {

    RegexDFAUTF8State* RegexDFAUTF8State::next (uint32_t character) {
        if (character < cSizeOfByte) {
            return m_bytes_transition[character];
        }
        unique_ptr<vector<Tree::Data>> result = m_tree_transitions.find(Interval(character, character));
        assert(result->size() <= 1);
        if (!result->empty()) {
            return result->front().m_value;
        }
        return nullptr;
    }

}
