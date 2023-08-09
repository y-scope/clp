#ifndef COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_AST_TPP
#define COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_AST_TPP

#include "RegexAST.hpp"

// C++ standard libraries
#include <algorithm>
#include <cassert>
#include <map>
#include <stack>

// Project headers
#include "../../spdlog_with_specializations.hpp"
#include "../Constants.hpp"
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

/// TODO: remove general `using` expressions like these from tpp
using std::map;
using std::max;
using std::min;
using std::pair;
using std::runtime_error;
using std::stack;
using std::unique_ptr;
using std::vector;

namespace compressor_frontend::finite_automata {

    template <typename NFAStateType>
    RegexASTLiteral<NFAStateType>::RegexASTLiteral (uint32_t character) : m_character(character) {

    }

    template <typename NFAStateType>
    void RegexASTLiteral<NFAStateType>::add (RegexNFA<NFAStateType>* nfa, NFAStateType* end_state) {
        nfa->add_root_interval(Interval(m_character, m_character), end_state);
    }

    template <typename NFAStateType>
    RegexASTInteger<NFAStateType>::RegexASTInteger (uint32_t digit) {
        digit = digit - '0';
        m_digits.push_back(digit);
    }

    template <typename NFAStateType>
    RegexASTInteger<NFAStateType>::RegexASTInteger (RegexASTInteger<NFAStateType>* left, uint32_t digit) {
        digit = digit - '0';
        m_digits = std::move(left->m_digits);
        m_digits.push_back(digit);
    }

    template <typename NFAStateType>
    void RegexASTInteger<NFAStateType>::add (RegexNFA<NFAStateType>* nfa, NFAStateType* end_state) {
        assert(false); // this shouldn't ever be called
    }

    template <typename NFAStateType>
    RegexASTOr<NFAStateType>::RegexASTOr (unique_ptr<RegexAST<NFAStateType>> left, unique_ptr<RegexAST<NFAStateType>> right) : m_left(std::move(left)),
                                                                                                                m_right(std::move(right)) {

    }

    template <typename NFAStateType>
    void RegexASTOr<NFAStateType>::add (RegexNFA<NFAStateType>* nfa, NFAStateType* end_state) {
        m_left->add(nfa, end_state);
        m_right->add(nfa, end_state);
    }

    template <typename NFAStateType>
    RegexASTCat<NFAStateType>::RegexASTCat (unique_ptr<RegexAST<NFAStateType>> left, unique_ptr<RegexAST<NFAStateType>> right) : m_left(std::move(left)),
                                                                                                                  m_right(std::move(right)) {

    }

    template <typename NFAStateType>
    void RegexASTCat<NFAStateType>::add (RegexNFA<NFAStateType>* nfa, NFAStateType* end_state) {
        NFAStateType* saved_root = nfa->m_root;
        NFAStateType* intermediate_state = nfa->new_state();
        m_left->add(nfa, intermediate_state);
        nfa->m_root = intermediate_state;
        m_right->add(nfa, end_state);
        nfa->m_root = saved_root;
    }

    template <typename NFAStateType>
    RegexASTMultiplication<NFAStateType>::RegexASTMultiplication (unique_ptr<RegexAST<NFAStateType>> operand, uint32_t min, uint32_t max) :
            m_operand(std::move(operand)), m_min(min), m_max(max) {

    }

    template <typename NFAStateType>
    void RegexASTMultiplication<NFAStateType>::add (RegexNFA<NFAStateType>* nfa, NFAStateType* end_state) {
        NFAStateType* saved_root = nfa->m_root;
        if (this->m_min == 0) {
            nfa->m_root->add_epsilon_transition(end_state);
        } else {
            for (int i = 1; i < this->m_min; i++) {
                NFAStateType* intermediate_state = nfa->new_state();
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
                NFAStateType* intermediate_state = nfa->new_state();
                m_operand->add(nfa, intermediate_state);
                nfa->m_root = intermediate_state;
            }
            for (uint32_t i = this->m_min + 1; i < this->m_max; i++) {
                m_operand->add(nfa, end_state);
                NFAStateType* intermediate_state = nfa->new_state();
                m_operand->add(nfa, intermediate_state);
                nfa->m_root = intermediate_state;
            }
            m_operand->add(nfa, end_state);
        }
        nfa->m_root = saved_root;
    }

    template <typename NFAStateType>
    RegexASTGroup<NFAStateType>::RegexASTGroup () {
        m_is_wildcard = false;
        m_negate = true;
    }

    template <typename NFAStateType>
    RegexASTGroup<NFAStateType>::RegexASTGroup (RegexASTGroup<NFAStateType>* left, RegexASTLiteral<NFAStateType>* right) {
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

    template <typename NFAStateType>
    RegexASTGroup<NFAStateType>::RegexASTGroup (RegexASTGroup<NFAStateType>* left, RegexASTGroup<NFAStateType>* right) {
        m_is_wildcard = false;
        m_negate = left->m_negate;
        m_ranges = left->m_ranges;
        assert(right->m_ranges.size() == 1); // Only add LiteralRange
        m_ranges.push_back(right->m_ranges[0]);
    }

    template <typename NFAStateType>
    RegexASTGroup<NFAStateType>::RegexASTGroup (RegexASTLiteral<NFAStateType>* right) {
        m_is_wildcard = false;
        if (right == nullptr) {
            SPDLOG_ERROR("A bracket expression in the schema contains illegal characters, remember to escape special characters. "
                         "Refer to README-Schema.md for more details.");
            throw runtime_error("RegexASTGroup2: right==nullptr");
        }
        m_negate = false;
        m_ranges.emplace_back(right->get_character(), right->get_character());
    }

    template <typename NFAStateType>
    RegexASTGroup<NFAStateType>::RegexASTGroup (RegexASTGroup<NFAStateType>* right) {
        m_is_wildcard = false;
        m_negate = false;
        assert(right->m_ranges.size() == 1); // Only add LiteralRange
        m_ranges.push_back(right->m_ranges[0]);
    }

    template <typename NFAStateType>
    RegexASTGroup<NFAStateType>::RegexASTGroup (RegexASTLiteral<NFAStateType>* left, RegexASTLiteral<NFAStateType>* right) {
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

    template <typename NFAStateType>
    RegexASTGroup<NFAStateType>::RegexASTGroup (const vector<uint32_t>& literals) {
        m_is_wildcard = false;
        m_negate = false;
        for (uint32_t literal: literals) {
            m_ranges.emplace_back(literal, literal);
        }
    }

    template <typename NFAStateType>
    RegexASTGroup<NFAStateType>::RegexASTGroup (uint32_t min, uint32_t max) {
        m_is_wildcard = false;
        m_negate = false;
        m_ranges.emplace_back(min, max);
    }

    // ranges must be sorted
    template <typename NFAStateType>
    vector<typename RegexASTGroup<NFAStateType>::Range> RegexASTGroup<NFAStateType>::merge (const vector<Range>& ranges) {
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
    template <typename NFAStateType>
    vector<typename RegexASTGroup<NFAStateType>::Range> RegexASTGroup<NFAStateType>::complement (const vector<Range>& ranges) {
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

    template <typename NFAStateType>
    void RegexASTGroup<NFAStateType>::add (RegexNFA<NFAStateType>* nfa, NFAStateType* end_state) {
        std::sort(this->m_ranges.begin(), this->m_ranges.end());
        vector<Range> merged = RegexASTGroup::merge(this->m_ranges);
        if (this->m_negate) {
            merged = RegexASTGroup::complement(merged);
        }
        for (const Range& r: merged) {
            nfa->m_root->add_interval(Interval(r.first, r.second), end_state);
        }
    }    
}

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_AST_TPP