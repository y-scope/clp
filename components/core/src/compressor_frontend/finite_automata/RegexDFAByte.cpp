#include "RegexDFAByte.hpp"

namespace compressor_frontend::finite_automata {

    void RegexDFAByteState::add_byte_transition (const uint8_t& byte, RegexDFAByteState*& dest_state) {
        m_bytes_transition[byte] = dest_state;
    }

    RegexDFAState* RegexDFAByteState::next (uint32_t character) {
        return m_bytes_transition[character];
    }

}
