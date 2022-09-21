#include "RegexAST.hpp"

// spdlog
#include <spdlog/spdlog.h>

// C++ standard libraries
#include <algorithm>
#include <cassert>
#include <map>
#include <stack>

// Project headers
#include "RegexNFA.hpp"
#include "UnicodeIntervalTree.hpp"

/* In order to use std::unordered_map (or absl::flat_hash_map) we need to have
 * a specialization for hash<std::set> from boost, abseil, etc. Afaik replacing
 * std::set (i.e. an ordered set) with an unordered set is difficult due to
 * fundamental issues of making an unordered data structure hashable.
 * (i.e. you need two containers with the same elements in differing orders to
 * hash to the same value, which makes computing/maintaining the hash of this
 * unordered container non-trivial)
 */

using std::map;
using std::max;
using std::min;
using std::pair;
using std::runtime_error;
using std::stack;
using std::unique_ptr;
using std::vector;

namespace compressor_frontend::finite_automata {


    RegexASTLiteral::RegexASTLiteral (uint32_t character) : m_character(character) {

    }

    void RegexASTLiteral::add (RegexNFA* nfa, RegexNFA::State* end_state) {
        nfa->add_root_interval(RegexNFA::Tree::Interval(m_character, m_character), end_state);
    }

    RegexASTInteger::RegexASTInteger (uint32_t digit) {
        digit = digit - '0';
        m_digits.push_back(digit);
    }

    RegexASTInteger::RegexASTInteger (RegexASTInteger* left, uint32_t digit) {
        digit = digit - '0';
        m_digits = std::move(left->m_digits);
        m_digits.push_back(digit);
    }

    void RegexASTInteger::add (RegexNFA* nfa, RegexNFA::State* end_state) {
        assert(false); // this shouldn't ever be called
    }


    RegexASTOr::RegexASTOr (unique_ptr<RegexAST> left, unique_ptr<RegexAST> right) : m_left(std::move(left)), m_right(std::move(right)) {

    }

    void RegexASTOr::add (RegexNFA* nfa, RegexNFA::State* end_state) {
        m_left->add(nfa, end_state);
        m_right->add(nfa, end_state);
    }

    RegexASTCat::RegexASTCat (unique_ptr<RegexAST> left, unique_ptr<RegexAST> right) : m_left(std::move(left)), m_right(std::move(right)) {

    }

    void RegexASTCat::add (RegexNFA* nfa, RegexNFA::State* end_state) {
        RegexNFA::State* saved_root = nfa->m_root;
        RegexNFA::State* intermediate_state = nfa->new_state();
        m_left->add(nfa, intermediate_state);
        nfa->m_root = intermediate_state;
        m_right->add(nfa, end_state);
        nfa->m_root = saved_root;
    }

    RegexASTMultiplication::RegexASTMultiplication (unique_ptr<RegexAST> operand, uint32_t min, uint32_t max) :
            m_operand(std::move(operand)), m_min(min), m_max(max) {

    }

    void RegexASTMultiplication::add (RegexNFA* nfa, RegexNFA::State* end_state) {
        RegexNFA::State* saved_root = nfa->m_root;
        if (this->m_min == 0) {
            nfa->m_root->add_epsilon_transition(end_state);
        } else {
            for (int i = 1; i < this->m_min; i++) {
                RegexNFA::State* intermediate_state = nfa->new_state();
                m_operand->add(nfa, intermediate_state);
                nfa->m_root = intermediate_state;
            }
            m_operand->add(nfa, end_state);
        }
        if (this->is_infinite()) {
            nfa->m_root = end_state;
            m_operand->add(nfa, end_state);
        } else if (this->m_max > this->m_min) {
            if (this->m_min != 0) {
                RegexNFA::State* intermediate_state = nfa->new_state();
                m_operand->add(nfa, intermediate_state);
                nfa->m_root = intermediate_state;
            }
            for (uint32_t i = this->m_min + 1; i < this->m_max; i++) {
                m_operand->add(nfa, end_state);
                RegexNFA::State* intermediate_state = nfa->new_state();
                m_operand->add(nfa, intermediate_state);
                nfa->m_root = intermediate_state;
            }
            m_operand->add(nfa, end_state);
        }
        nfa->m_root = saved_root;
    }

    RegexASTGroup::RegexASTGroup () {
        m_is_wildcard = false;
        m_negate = true;
    }

    RegexASTGroup::RegexASTGroup (RegexASTGroup* left, RegexASTLiteral* right) {
        m_is_wildcard = false;
        if (right == nullptr) {
            SPDLOG_ERROR("A bracket expression in the schema contains illegal characters, remember to escape special characters. "
                         "Refer to README-Schema.md for more details.");
            throw runtime_error("RegexASTGroup1: right==nullptr");
        }
        m_negate = left->m_negate;
        m_ranges = left->m_ranges;
        m_ranges.emplace_back(right->get_character(), right->get_character());
    }

    RegexASTGroup::RegexASTGroup (RegexASTGroup* left, RegexASTGroup* right) {
        m_is_wildcard = false;
        m_negate = left->m_negate;
        m_ranges = left->m_ranges;
        assert(right->m_ranges.size() == 1); // Only add LiteralRange
        m_ranges.push_back(right->m_ranges[0]);
    }

    RegexASTGroup::RegexASTGroup (RegexASTLiteral* right) {
        m_is_wildcard = false;
        if (right == nullptr) {
            SPDLOG_ERROR("A bracket expression in the schema contains illegal characters, remember to escape special characters. "
                         "Refer to README-Schema.md for more details.");
            throw runtime_error("RegexASTGroup2: right==nullptr");
        }
        m_negate = false;
        m_ranges.emplace_back(right->get_character(), right->get_character());
    }

    RegexASTGroup::RegexASTGroup (RegexASTGroup* right) {
        m_is_wildcard = false;
        m_negate = false;
        assert(right->m_ranges.size() == 1); // Only add LiteralRange
        m_ranges.push_back(right->m_ranges[0]);
    }

    RegexASTGroup::RegexASTGroup (RegexASTLiteral* left, RegexASTLiteral* right) {
        m_is_wildcard = false;
        if (left == nullptr || right == nullptr) {
            SPDLOG_ERROR("A bracket expression in the schema contains illegal characters, remember to escape special characters. "
                         "Refer to README-Schema.md for more details.");
            throw runtime_error("RegexASTGroup3: left == nullptr || right == nullptr");
        }
        m_negate = false;
        assert(right->get_character() > left->get_character());
        m_ranges.emplace_back(left->get_character(), right->get_character());
    }

    RegexASTGroup::RegexASTGroup (const vector<uint32_t>& literals) {
        m_is_wildcard = false;
        m_negate = false;
        for (uint32_t literal: literals) {
            m_ranges.emplace_back(literal, literal);
        }
    }

    RegexASTGroup::RegexASTGroup (uint32_t min, uint32_t max) {
        m_is_wildcard = false;
        m_negate = false;
        m_ranges.emplace_back(min, max);
    }

    void RegexASTGroup::add_range (uint32_t min, uint32_t max) {
        m_ranges.emplace_back(min, max);
    }

    void RegexASTGroup::add_literal (uint32_t literal) {
        m_ranges.emplace_back(literal, literal);
    }

// ranges must be sorted
    vector<RegexASTGroup::Range> RegexASTGroup::merge (const vector<Range>& ranges) {
        vector<Range> merged;
        if (ranges.empty()) {
            return merged;
        }
        Range cur = ranges[0];
        for (size_t i = 1; i < ranges.size(); i++) {
            Range r = ranges[i];
            if (r.first <= cur.second + 1) {
                cur.second = max(r.second, cur.second);
            } else {
                merged.push_back(cur);
                cur = r;
            }
        }
        merged.push_back(cur);
        return merged;
    }

// ranges must be sorted and non-overlapping
    vector<RegexASTGroup::Range> RegexASTGroup::complement (const vector<Range>& ranges) {
        vector<Range> complemented;
        uint32_t low = 0;
        for (const Range& r: ranges) {
            if (r.first > 0) {
                complemented.emplace_back(low, r.first - 1);
            }
            low = r.second + 1;
        }
        if (low > 0) {
            complemented.emplace_back(low, cUnicodeMax);
        }
        return complemented;
    }

    void RegexASTGroup::add (RegexNFA* nfa, RegexNFA::State* end_state) {
        std::sort(this->m_ranges.begin(), this->m_ranges.end());
        vector<Range> merged = RegexASTGroup::merge(this->m_ranges);
        if (this->m_negate) {
            merged = RegexASTGroup::complement(merged);
        }
        for (const Range& r: merged) {
            nfa->m_root->add_interval(RegexNFA::Tree::Interval(r.first, r.second), end_state);
        }
    }    
}