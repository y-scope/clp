#include "RegexNFAByteState.hpp"

namespace compressor_frontend::finite_automata {
    void RegexNFAByteState::add_interval (Interval interval, RegexNFAByteState* dest_state) {
        if (interval.first < cSizeOfByte) {
            uint32_t bound = min(interval.second, cSizeOfByte - 1);
            for (uint32_t i = interval.first; i <= bound; i++) {
                add_byte_transition(i, dest_state);
            }
            interval.first = bound + 1;
        }
    }
}