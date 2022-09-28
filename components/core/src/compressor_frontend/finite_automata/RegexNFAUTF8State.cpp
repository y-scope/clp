#include "RegexNFAUTF8State.hpp"

// Project headers
#include "../Constants.hpp"

namespace compressor_frontend::finite_automata {
    void RegexNFAUTF8State::add_interval (Interval interval, RegexNFAUTF8State* dest_state) {
        RegexNFAByteState::add_interval(interval, dest_state);
        if (interval.second < cSizeOfByte) {
            return;
        }
        unique_ptr<vector<Tree::Data>> overlaps = m_tree_transitions.pop(interval);
        for (const Tree::Data& data: *overlaps) {
            uint32_t overlap_low = max(data.m_interval.first, interval.first);
            uint32_t overlap_high = min(data.m_interval.second, interval.second);

            std::vector<RegexNFAUTF8State*> tree_states = data.m_value;
            tree_states.push_back(dest_state);
            m_tree_transitions.insert(Interval(overlap_low, overlap_high), tree_states);
            if (data.m_interval.first < interval.first) {
                m_tree_transitions.insert(Interval(data.m_interval.first, interval.first - 1), data.m_value);
            } else if (data.m_interval.first > interval.first) {
                m_tree_transitions.insert(Interval(interval.first, data.m_interval.first - 1), {dest_state});
            }
            if (data.m_interval.second > interval.second) {
                m_tree_transitions.insert(Interval(interval.second + 1, data.m_interval.second), data.m_value);
            }
            interval.first = data.m_interval.second + 1;
        }
        if (interval.first != 0 && interval.first <= interval.second) {
            m_tree_transitions.insert(interval, {dest_state});
        }
    }
}
