#include "FiniteAutomata.hpp"

// spdlog
#include <spdlog/spdlog.h>

// C++ standard libraries
#include <algorithm>
#include <cassert>
#include <map>
#include <stack>

// Project headers
#include "Constants.hpp"

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
using std::move;
using std::pair;
using std::runtime_error;
using std::stack;
using std::unique_ptr;
using std::vector;

namespace compressor_frontend {
    RegexDFA::State* RegexDFA::State::next (uint32_t c) {
        if (c < cSizeOfByte) {
            return this->byte_transitions[c];
        }
        unique_ptr<vector<Tree::Data>> result = this->tree_transitions.find(Tree::Interval(c, c));
        assert(result->size() <= 1);
        if (!result->empty()) {
            return result->front().value;
        }
        return nullptr;
    }

    RegexASTLiteral::RegexASTLiteral (uint32_t character) : character(character) {

    }

    void RegexASTLiteral::add_state (RegexNFA* nfa, RegexNFA::State* end_state) {
        nfa->add_root_interval(RegexNFA::Tree::Interval(this->character, this->character), end_state);
    }

    RegexASTInteger::RegexASTInteger (uint32_t digit) {
        digit = digit - '0';
        digits.push_back(digit);
    }

    RegexASTInteger::RegexASTInteger (RegexASTInteger* left, uint32_t digit) {
        digit = digit - '0';
        digits = move(left->digits);
        digits.push_back(digit);
    }

    void RegexASTInteger::add_state (RegexNFA* nfa, RegexNFA::State* end_state) {
        assert(false); // this shouldn't ever be called
    }


    RegexASTOr::RegexASTOr (unique_ptr<RegexAST> left, unique_ptr<RegexAST> right) : left(move(left)), right(move(right)) {

    }

    void RegexASTOr::add_state (RegexNFA* nfa, RegexNFA::State* end_state) {
        left->add_state(nfa, end_state);
        right->add_state(nfa, end_state);
    }

    RegexASTCat::RegexASTCat (unique_ptr<RegexAST> left, unique_ptr<RegexAST> right) : left(move(left)), right(move(right)) {

    }

    void RegexASTCat::add_state (RegexNFA* nfa, RegexNFA::State* end_state) {
        RegexNFA::State* saved_root = nfa->root;
        RegexNFA::State* intermediate_state = nfa->new_state();
        left->add_state(nfa, intermediate_state);
        nfa->root = intermediate_state;
        right->add_state(nfa, end_state);
        nfa->root = saved_root;
    }

    RegexASTMultiplication::RegexASTMultiplication (unique_ptr<RegexAST> operand, uint32_t min, uint32_t max) :
            operand(move(operand)), min(min), max(max) {

    }

    void RegexASTMultiplication::add_state (RegexNFA* nfa, RegexNFA::State* end_state) {
        RegexNFA::State* saved_root = nfa->root;
        if (this->min == 0) {
            nfa->root->add_epsilon_transition(end_state);
        } else {
            for (int i = 1; i < this->min; i++) {
                RegexNFA::State* intermediate_state = nfa->new_state();
                operand->add_state(nfa, intermediate_state);
                nfa->root = intermediate_state;
            }
            operand->add_state(nfa, end_state);
        }
        if (this->is_infinite()) {
            nfa->root = end_state;
            operand->add_state(nfa, end_state);
        } else if (this->max > this->min) {
            if (this->min != 0) {
                RegexNFA::State* intermediate_state = nfa->new_state();
                operand->add_state(nfa, intermediate_state);
                nfa->root = intermediate_state;
            }
            for (uint32_t i = this->min + 1; i < this->max; i++) {
                operand->add_state(nfa, end_state);
                RegexNFA::State* intermediate_state = nfa->new_state();
                operand->add_state(nfa, intermediate_state);
                nfa->root = intermediate_state;
            }
            operand->add_state(nfa, end_state);
        }
        nfa->root = saved_root;
    }

    RegexASTGroup::RegexASTGroup () {
        is_wildcard = false;
        negate = true;
    }

    RegexASTGroup::RegexASTGroup (RegexASTGroup* left, RegexASTLiteral* right) {
        is_wildcard = false;
        if (right == nullptr) {
            SPDLOG_ERROR("A bracket expression in the schema contains illegal characters, remember to escape special characters. "
                         "Refer to README-Schema.md for more details.");
            throw runtime_error("RegexASTGroup1: right==nullptr");
        }
        negate = left->negate;
        ranges = left->ranges;
        ranges.emplace_back(right->get_character(), right->get_character());
    }

    RegexASTGroup::RegexASTGroup (RegexASTGroup* left, RegexASTGroup* right) {
        is_wildcard = false;
        negate = left->negate;
        ranges = left->ranges;
        assert(right->ranges.size() == 1); // Only add LiteralRange
        ranges.push_back(right->ranges[0]);
    }

    RegexASTGroup::RegexASTGroup (RegexASTLiteral* right) {
        is_wildcard = false;
        if (right == nullptr) {
            SPDLOG_ERROR("A bracket expression in the schema contains illegal characters, remember to escape special characters. "
                         "Refer to README-Schema.md for more details.");
            throw runtime_error("RegexASTGroup2: right==nullptr");
        }
        negate = false;
        ranges.emplace_back(right->get_character(), right->get_character());
    }

    RegexASTGroup::RegexASTGroup (RegexASTGroup* right) {
        is_wildcard = false;
        negate = false;
        assert(right->ranges.size() == 1); // Only add LiteralRange
        ranges.push_back(right->ranges[0]);
    }

    RegexASTGroup::RegexASTGroup (RegexASTLiteral* left, RegexASTLiteral* right) {
        is_wildcard = false;
        if (left == nullptr || right == nullptr) {
            SPDLOG_ERROR("A bracket expression in the schema contains illegal characters, remember to escape special characters. "
                         "Refer to README-Schema.md for more details.");
            throw runtime_error("RegexASTGroup3: left == nullptr || right == nullptr");
        }
        negate = false;
        assert(right->get_character() > left->get_character());
        ranges.emplace_back(left->get_character(), right->get_character());
    }

    RegexASTGroup::RegexASTGroup (const vector<uint32_t>& literals) {
        is_wildcard = false;
        negate = false;
        for (uint32_t literal: literals) {
            ranges.emplace_back(literal, literal);
        }
    }

    RegexASTGroup::RegexASTGroup (uint32_t min, uint32_t max) {
        is_wildcard = false;
        negate = false;
        ranges.emplace_back(min, max);
    }

    void RegexASTGroup::add_range (uint32_t min, uint32_t max) {
        ranges.emplace_back(min, max);
    }

    void RegexASTGroup::add_literal (uint32_t literal) {
        ranges.emplace_back(literal, literal);
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

    void RegexASTGroup::add_state (RegexNFA* nfa, RegexNFA::State* end_state) {
        std::sort(this->ranges.begin(), this->ranges.end());
        vector<Range> merged = RegexASTGroup::merge(this->ranges);
        if (this->negate) {
            merged = RegexASTGroup::complement(merged);
        }
        for (const Range& r: merged) {
            nfa->root->add_interval(RegexNFA::Tree::Interval(r.first, r.second), end_state);
        }
    }

    RegexNFA::RegexNFA () {
        this->root = new_state();
    }

    RegexNFA::State* RegexNFA::new_state () {
        unique_ptr<State> ptr(new State());
        State* state = ptr.get();
        this->states.push_back(move(ptr));
        return state;
    }

    void RegexNFA::State::add_interval (Tree::Interval interval, State* state) {
        if (interval.first < cSizeOfByte) {
            uint32_t bound = min(interval.second, cSizeOfByte - 1);
            for (uint32_t i = interval.first; i <= bound; i++) {
                this->bytes_transitions[i].push_back(state);
            }
            interval.first = bound + 1;
        }
        if (interval.first > interval.second) {
            return;
        }

        unique_ptr<vector<Tree::Data>> overlaps = this->tree_transitions.pop(interval);
        for (const Tree::Data& data: *overlaps) {
            uint32_t overlap_low = max(data.interval.first, interval.first);
            uint32_t overlap_high = min(data.interval.second, interval.second);

            StateVec tree_states = data.value;
            tree_states.push_back(state);
            this->tree_transitions.insert(Tree::Interval(overlap_low, overlap_high), tree_states);
            if (data.interval.first < interval.first) {
                this->tree_transitions.insert(Tree::Interval(data.interval.first, interval.first - 1), data.value);
            } else if (data.interval.first > interval.first) {
                this->tree_transitions.insert(Tree::Interval(interval.first, data.interval.first - 1), {state});
            }
            if (data.interval.second > interval.second) {
                this->tree_transitions.insert(Tree::Interval(interval.second + 1, data.interval.second), data.value);
            }
            interval.first = data.interval.second + 1;
        }
        if (interval.first != 0 && interval.first <= interval.second) {
            this->tree_transitions.insert(interval, {state});
        }
    }

    void RegexNFA::epsilon_closure (StateSet* epsilon_closure, State* state) {
        stack<State*> stack;
        stack.push(state);
        while (!stack.empty()) {
            State* t = stack.top();
            stack.pop();
            if (epsilon_closure->insert(t).second) {
                for (State* const u: t->get_epsilon_transitions()) {
                    stack.push(u);
                }
            }
        }
    }

    RegexDFA::State* RegexDFA::new_state (const RegexNFA::StateSet* set) {
        unique_ptr<State> ptr(new State());
        State* state = ptr.get();
        this->states.push_back(move(ptr));

        for (const RegexNFA::State* s: *set) {
            if (s->is_accepting()) {
                state->terminals.push_back(s->get_tag());
            }
        }
        return state;
    }

    unique_ptr<RegexDFA> RegexNFA::to_dfa () {
        unique_ptr<RegexDFA> dfa(new RegexDFA);
        map<StateSet, RegexDFA::State*> dfa_states;
        stack<StateSet> unmarked_sets;

        auto create_dfa_state =
                [&dfa, &dfa_states, &unmarked_sets] (const StateSet& set) -> RegexDFA::State* {
                    RegexDFA::State* state = dfa->new_state(&set);
                    dfa_states[set] = state;
                    unmarked_sets.push(set);
                    return state;
                };

        StateSet start_set;
        RegexNFA::epsilon_closure(&start_set, this->root);
        create_dfa_state(start_set);

        while (!unmarked_sets.empty()) {
            StateSet set = unmarked_sets.top();
            unmarked_sets.pop();
            RegexDFA::State* dfa_state = dfa_states.at(set);

            map<uint32_t, StateSet> ascii_transitions_map;
            map<Tree::Interval, StateSet> transitions_map;

            for (State* s0: set) {
                for (uint32_t i = 0; i < cSizeOfByte; i++) {
                    for (State* const s1: s0->get_byte_transitions(i)) {
                        this->epsilon_closure(&ascii_transitions_map[i], s1);
                    }
                }
                for (const Tree::Data& data: s0->get_tree_transitions().all()) {
                    for (State* const s1: data.value) {
                        this->epsilon_closure(&transitions_map[data.interval], s1);
                    }
                }
            }

            auto next_dfa_state =
                    [&dfa_states, &create_dfa_state] (const StateSet& set) -> RegexDFA::State* {
                        RegexDFA::State* state;
                        auto it = dfa_states.find(set);
                        if (it == dfa_states.end()) {
                            state = create_dfa_state(set);
                        } else {
                            state = it->second;
                        }
                        return state;
                    };

            for (const map<uint32_t, StateSet>::value_type& kv: ascii_transitions_map) {
                dfa_state->byte_transitions[kv.first] = next_dfa_state(kv.second);
            }
            for (const map<Tree::Interval, StateSet>::value_type& kv: transitions_map) {
                dfa_state->tree_transitions.insert(kv.first, next_dfa_state(kv.second));
            }
        }
        return dfa;
    }

    void RegexNFA::reverse () {
        // add new end with all accepting pointing to it
        State* new_end = new_state();
        for (unique_ptr<State>& state_ptr: states) {
            if (state_ptr->is_accepting()) {
                state_ptr->add_epsilon_transition(new_end);
                state_ptr->set_accepting(false);
            }
        }
        // move edges
        map<pair<State*, State*>, vector<uint8_t>> byte_edges;
        map<pair<State*, State*>, bool> epsilon_edges;
        for (unique_ptr<State>& src_state_ptr: states) {
            src_state_ptr->reset_tree_transitions();
            for (uint32_t byte = 0; byte < cSizeOfByte; byte++) {
                for (State* dest_state_ptr: src_state_ptr->get_byte_transitions(byte)) {
                    byte_edges[pair<State*, State*>(src_state_ptr.get(), dest_state_ptr)].push_back(byte);
                }
                src_state_ptr->clear_byte_transitions(byte);
            }
            for (State* dest_state_ptr: src_state_ptr->get_epsilon_transitions()) {
                epsilon_edges[pair<State*, State*>(src_state_ptr.get(), dest_state_ptr)] = true;
            }
            src_state_ptr->clear_epsilon_transitions();
        }

        // reverse edges
        for (unique_ptr<State>& src_state_ptr: states) {
            for (unique_ptr<State>& dest_state_ptr: states) {
                pair<State*, State*> key(src_state_ptr.get(), dest_state_ptr.get());
                auto byte_it = byte_edges.find(key);
                if (byte_it != byte_edges.end()) {
                    for (uint8_t byte: byte_it->second) {
                        dest_state_ptr->add_byte_transition(byte, src_state_ptr.get());
                    }
                }
                auto epsilon_it = epsilon_edges.find(key);
                if (epsilon_it != epsilon_edges.end()) {
                    dest_state_ptr->add_epsilon_transition(src_state_ptr.get());
                }
            }
        }

        // propagate tag from old accepting states
        for (State* old_accepting_state: new_end->get_epsilon_transitions()) {
            int tag = old_accepting_state->get_tag();
            stack<State*> unvisited_states;
            std::set<State*> visited_states;
            unvisited_states.push(old_accepting_state);
            while (!unvisited_states.empty()) {
                State* current_state = unvisited_states.top();
                current_state->set_tag(tag);
                unvisited_states.pop();
                visited_states.insert(current_state);
                for(uint32_t byte  = 0; byte < cSizeOfByte; byte++) {
                    StateVec byte_transitions = current_state->get_byte_transitions(byte);
                    for (State* next_state: byte_transitions) {
                        if (visited_states.find(next_state) == visited_states.end()) {
                            unvisited_states.push(next_state);
                        }
                    }
                }
                for (State* next_state: current_state->get_epsilon_transitions()) {
                    if (visited_states.find(next_state) == visited_states.end()) {
                        unvisited_states.push(next_state);
                    }
                }
            }
        }
        for (int32_t i = states.size() - 1; i >= 0; i--) {
            unique_ptr<State>& src_state_unique_ptr = states[i];
            State* src_state = src_state_unique_ptr.get();
            int tag = src_state->get_tag();
            for(uint32_t byte  = 0; byte < cSizeOfByte; byte++) {
                StateVec byte_transitions = src_state->get_byte_transitions(byte);
                for (int32_t j = byte_transitions.size() - 1; j >= 0; j--) {
                    State*& dest_state = byte_transitions[j];
                    if (dest_state == root) {
                        dest_state = new_state();
                        assert(dest_state != nullptr);
                        dest_state->set_tag(tag);
                        dest_state->set_accepting(true);
                    }
                }
            }
            for (State* dest_state: src_state->get_epsilon_transitions()) {
                if (dest_state == root) {
                    dest_state = new_state();
                    dest_state->set_tag(src_state->get_tag());
                    dest_state->set_accepting(true);
                }
            }
        }

        for (uint32_t i = 0; i < states.size(); i++) {
            if (states[i].get() == root) {
                states.erase(states.begin() + i);
                break;
            }
        }
        // start from the end
        root = new_end;

    }


    template<class T>
    void UnicodeIntervalTree<T>::insert (Interval interval, T value) {
        this->root = Node::insert(move(this->root), interval, value);
    }

    template<class T>
    unique_ptr<class UnicodeIntervalTree<T>::Node> UnicodeIntervalTree<T>::Node::insert (unique_ptr<Node> node, Interval interval, T value) {
        if (node == nullptr) {
            unique_ptr<Node> n(new Node(interval, value));
            n->update();
            return n;
        }
        if (interval < node->interval) {
            node->left = Node::insert(move(node->left), interval, value);
        } else if (interval > node->interval) {
            node->right = Node::insert(move(node->right), interval, value);
        } else {
            node->value = value;
        }
        node->update();
        return Node::balance(move(node));
    }

    template<typename T>
    vector<typename UnicodeIntervalTree<T>::Data> UnicodeIntervalTree<T>::all () const {
        vector<Data> results;
        if (this->root != nullptr) {
            this->root->all(&results);
        }
        return results;
    }

    template<typename T>
    void UnicodeIntervalTree<T>::Node::all (vector<Data>* results) {
        if (this->left != nullptr) {
            this->left->all(results);
        }
        results->push_back(Data(this->interval, this->value));
        if (this->right != nullptr) {
            this->right->all(results);
        }
    }

    template<typename T>
    unique_ptr<vector<typename UnicodeIntervalTree<T>::Data>> UnicodeIntervalTree<T>::find (Interval interval) {
        unique_ptr<vector<Data>> results(new vector<Data>);
        this->root->find(interval, results.get());
        return results;
    }

    template<class T>
    void UnicodeIntervalTree<T>::Node::find (Interval interval, vector<Data>* results) {
        if (!this->overlaps_recursive(interval)) {
            return;
        }
        if (this->left != nullptr) {
            this->left->find(interval, results);
        }
        if (this->overlaps(interval)) {
            results->push_back(Data(this->interval, this->value));
        }
        if (this->right != nullptr) {
            this->right->find(interval, results);
        }
    }

    template<class T>
    unique_ptr<vector<typename UnicodeIntervalTree<T>::Data>> UnicodeIntervalTree<T>::pop (Interval interval) {
        unique_ptr<vector<Data>> results(new vector<Data>);
        while (true) {
            unique_ptr<Node> n;
            this->root = Node::pop(move(this->root), interval, &n);
            if (n == nullptr) {
                break;
            }
            results->push_back(Data(n->get_interval(), n->get_value()));
        }
        return results;
    }

    template<class T>
    unique_ptr<typename UnicodeIntervalTree<T>::Node> UnicodeIntervalTree<T>::Node::pop (unique_ptr<Node> node, Interval interval,
                                                                                              unique_ptr<Node>* ret) {
        if (node == nullptr) {
            return nullptr;
        }
        if (!node->overlaps_recursive(interval)) {
            return node;
        }
        node->left = Node::pop(move(node->left), interval, ret);
        if (ret->get() != nullptr) {
            node->update();
            return Node::balance(move(node));
        }
        assert(node->overlaps(interval));
        ret->reset(node.release());
        if (((*ret)->left == nullptr) && ((*ret)->right == nullptr)) {
            return nullptr;
        } else if ((*ret)->left == nullptr) {
            return move((*ret)->right);
        } else if ((*ret)->right == nullptr) {
            return move((*ret)->left);
        } else {
            unique_ptr<Node> replacement;
            unique_ptr<Node> sub_tree = Node::pop_min(move((*ret)->right), &replacement);
            replacement->left = move((*ret)->left);
            replacement->right = move(sub_tree);
            replacement->update();
            return Node::balance(move(replacement));
        }
    }

    template<class T>
    unique_ptr<typename UnicodeIntervalTree<T>::Node> UnicodeIntervalTree<T>::Node::pop_min (unique_ptr<Node> node, unique_ptr<Node>* ret) {
        assert(node != nullptr);
        if (node->left == nullptr) {
            assert(node->right != nullptr);
            unique_ptr<Node> right(move(node->right));
            ret->reset(node.release());
            return right;
        }
        node->left = Node::pop_min(move(node->left), ret);
        node->update();
        return Node::balance(move(node));
    }

    template<class T>
    void UnicodeIntervalTree<T>::Node::update () {
        if ((this->left == nullptr) && (this->right == nullptr)) {
            this->height = 1;
            this->lower = this->interval.first;
            this->upper = this->interval.second;
        } else if (this->left == nullptr) {
            this->height = 2;
            this->lower = this->interval.first;
            this->upper = max(this->interval.second, this->right->upper);
        } else if (this->right == nullptr) {
            this->height = 2;
            this->lower = this->left->lower;
            this->upper = max(this->interval.second, this->left->upper);
        } else {
            this->height = max(this->left->height, this->right->height) + 1;
            this->lower = this->left->lower;
            this->upper = max({this->interval.second, this->left->upper, this->right->upper});
        }
    }

    template<class T>
    int UnicodeIntervalTree<T>::Node::balance_factor () {
        return (this->right != nullptr ? this->right.get() : 0) -
               (this->left != nullptr ? this->left.get() : 0);
    }

    template<class T>
    unique_ptr<typename UnicodeIntervalTree<T>::Node> UnicodeIntervalTree<T>::Node::balance (unique_ptr<Node> node) {
        int factor = node->balance_factor();
        if (factor * factor <= 1) {
            return node;
        }
        int sub_factor = (factor < 0) ? node->left->balance_factor() : node->right->balance_factor();
        if (factor * sub_factor > 0) {
            return Node::rotate(move(node), factor);
        }
        if (factor == 2) {
            node->right = Node::rotate(move(node->right), sub_factor);
        } else {
            node->left = Node::rotate(move(node->left), sub_factor);
        }
        return Node::rotate(move(node), factor);
    }

    template<class T>
    unique_ptr<typename UnicodeIntervalTree<T>::Node> UnicodeIntervalTree<T>::Node::rotate (unique_ptr<Node> node, int factor) {
        if (factor < 0) {
            return Node::rotate_cw(move(node));
        } else if (factor > 0) {
            return Node::rotate_ccw(move(node));
        }
        return node;
    }

    template<class T>
    unique_ptr<typename UnicodeIntervalTree<T>::Node> UnicodeIntervalTree<T>::Node::rotate_cw (unique_ptr<Node> node) {
        unique_ptr<Node> n(move(node->left));
        node->left.reset(n->right.release());
        n->right.reset(node.release());
        n->right->update();
        n->update();
        return n;
    }

    template<class T>
    unique_ptr<typename UnicodeIntervalTree<T>::Node> UnicodeIntervalTree<T>::Node::rotate_ccw (unique_ptr<Node> node) {
        unique_ptr<Node> n(move(node->right));
        node->right.reset(n->left.release());
        n->left.reset(node.release());
        n->left->update();
        n->update();
        return n;
    }

    template<class T>
    bool UnicodeIntervalTree<T>::Node::overlaps_recursive (Interval i) {
        return ((this->lower <= i.first) && (i.first <= this->upper)) ||
               ((this->lower <= i.second) && (i.second <= this->upper)) ||
               ((i.first <= this->lower) && (this->lower <= i.second));
    }

    template<class T>
    bool UnicodeIntervalTree<T>::Node::overlaps (Interval i) {
        return ((this->interval.first <= i.first) && (i.first <= this->interval.second)) ||
               ((this->interval.first <= i.second) && (i.second <= this->interval.second)) ||
               ((i.first <= this->interval.first) && (this->interval.first <= i.second));
    }
}