#ifndef FINITEAUTOMATA_HPP
#define FINITEAUTOMATA_HPP

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
#include <set>
#include <algorithm>

template <class T>
class UnicodeIntervalTree {
public:
    typedef std::pair<uint32_t, uint32_t> Interval;

    struct Data {
    public:
        Interval interval;
        T value;
        Data(Interval interval, T value) : interval(std::move(interval)), value(value) { }
    };

    void insert(Interval interval, T value);
    std::vector<Data> all();
    std::unique_ptr<std::vector<Data>> find(Interval interval);
    std::unique_ptr<std::vector<Data>> pop(Interval interval);
    void reset() {
        root.reset();
    }

private:
    class Node {
    public:
        Interval interval;
        T value;
        uint32_t lower{};
        uint32_t upper{};
        int height{};
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;

        static std::unique_ptr<Node> balance(std::unique_ptr<Node> node);
        static std::unique_ptr<Node> insert(std::unique_ptr<Node> node, Interval interval, T value);
        static std::unique_ptr<Node> pop(std::unique_ptr<Node> node, Interval interval, std::unique_ptr<Node>* ret);
        static std::unique_ptr<Node> pop_min(std::unique_ptr<Node> node, std::unique_ptr<Node>* ret);
        static std::unique_ptr<Node> rotate(std::unique_ptr<Node> node, int factor);
        static std::unique_ptr<Node> rotate_cw(std::unique_ptr<Node> node);
        static std::unique_ptr<Node> rotate_ccw(std::unique_ptr<Node> node);
        Node() : lower(0), upper(0), height(0) {}
        Node(Interval i, T v) : interval(std::move(i)), value(v) { }
        void all(std::vector<Data>* results);
        void find(Interval interval, std::vector<Data>* results);
        void update();
        int balance_factor();
        bool overlaps_recursive(Interval i);
        bool overlaps(Interval i);
    };

    std::unique_ptr<Node> root;
};

class RegexDFA;

class RegexNFA {
public:
    static uint32_t const SIZE_OF_BYTE = 256;
    class State;
    typedef std::vector<State*> StateVec;
    typedef std::set<State*> StateSet;
    typedef UnicodeIntervalTree<StateVec> Tree;
    struct State {
        bool accepting;
        int tag;
        StateVec byte_transitions[RegexNFA::SIZE_OF_BYTE];
        Tree tree_transitions;
        StateVec epsilon;
        void add(Tree::Interval interval, State *state);
    };
    State* root;
    RegexNFA();
    State* new_state();
    std::unique_ptr<RegexDFA> to_dfa();
    void reverse();
private:
    std::vector<std::unique_ptr<State>> states;
    static void epsilon_closure(StateSet* epsilon_closure, State* state);
};

class RegexDFA {
public:
    class State;
    typedef UnicodeIntervalTree<State*> Tree;

    State* root() { return this->states.at(0).get(); }
    State* new_state(const RegexNFA::StateSet* set);

    struct State {
    public:
        std::vector<int> terminals;
        State* byte_transitions[RegexNFA::SIZE_OF_BYTE];
        Tree tree_transitions;

        State* next(uint32_t c);
    };

private:
    std::vector<std::unique_ptr<State>> states;
};

class RegexAST {
public:
    virtual void add_state(RegexNFA* nfa, RegexNFA::State* end_state) = 0;
    virtual ~RegexAST() = default;
    [[nodiscard]] virtual RegexAST* clone () const = 0;
    virtual void set_possible_inputs_to_true (bool is_possible_input[]) const = 0;
    virtual void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) = 0;
};


class RegexASTLiteral : public RegexAST {
public:
    uint32_t character;

    explicit RegexASTLiteral(uint32_t character);
    void add_state(RegexNFA* nfa, RegexNFA::State* end_state) override;
    [[nodiscard]] RegexASTLiteral* clone () const override {
        return new RegexASTLiteral (*this);
    }
    void set_possible_inputs_to_true (bool is_possible_input[]) const override {
        is_possible_input[character] = true;
    }
    void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) override {
        // DO NOTHING
    }

};

class RegexASTInteger : public RegexAST {
public:
    std::vector<uint32_t> digits;

    explicit RegexASTInteger(uint32_t digit);
    RegexASTInteger(RegexASTInteger* left, uint32_t digit);
    void add_state(RegexNFA* nfa, RegexNFA::State* end_state) override;
    [[nodiscard]] RegexASTInteger* clone () const override {
        return new RegexASTInteger (*this);
    }
    void set_possible_inputs_to_true (bool is_possible_input[]) const override {
        for (uint32_t i: digits) {
            is_possible_input[i + '0'] = true;
        }
    }
    void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) override {
        // DO NOTHING
    }
};

class RegexASTOr : public RegexAST {
public:
    std::unique_ptr<RegexAST> left;
    std::unique_ptr<RegexAST> right;

    RegexASTOr(std::unique_ptr<RegexAST>, std::unique_ptr<RegexAST>);
    void add_state(RegexNFA* nfa, RegexNFA::State* end_state) override;
    RegexASTOr (RegexASTOr const& rhs) {
        left = std::unique_ptr<RegexAST>(rhs.left->clone());
        right = std::unique_ptr<RegexAST>(rhs.right->clone());
    }
    [[nodiscard]] RegexASTOr* clone () const override {
        return new RegexASTOr (*this);
    }
    void set_possible_inputs_to_true (bool is_possible_input[]) const override {
        left->set_possible_inputs_to_true(is_possible_input);
        right->set_possible_inputs_to_true(is_possible_input);
    }
    void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) override {
        left->remove_delimiters_from_wildcard(delimiters);
        right->remove_delimiters_from_wildcard(delimiters);
    }
};

class RegexASTCat : public RegexAST {
public:
    std::unique_ptr<RegexAST> left;
    std::unique_ptr<RegexAST> right;

    RegexASTCat(std::unique_ptr<RegexAST>, std::unique_ptr<RegexAST>);
    void add_state(RegexNFA* nfa, RegexNFA::State* end_state) override;
    RegexASTCat (RegexASTCat const& rhs) {
        left = std::unique_ptr<RegexAST>(rhs.left->clone());
        right = std::unique_ptr<RegexAST>(rhs.right->clone());
    }
    [[nodiscard]] RegexASTCat* clone () const override {
        return new RegexASTCat (*this);
    }
    void set_possible_inputs_to_true (bool is_possible_input[]) const override {
        left->set_possible_inputs_to_true(is_possible_input);
        right->set_possible_inputs_to_true(is_possible_input);
    }
    void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) override {
        left->remove_delimiters_from_wildcard(delimiters);
        right->remove_delimiters_from_wildcard(delimiters);
    }
};

class RegexASTMultiplication : public RegexAST {
public:
    std::unique_ptr<RegexAST> operand;
    uint32_t min;
    uint32_t max;

    [[nodiscard]] bool is_infinite() const {
        return this->max == 0;
    }
    RegexASTMultiplication(std::unique_ptr<RegexAST>, uint32_t, uint32_t);
    void add_state(RegexNFA* nfa, RegexNFA::State* end_state) override;
    RegexASTMultiplication (RegexASTMultiplication const& rhs) {
        operand = std::unique_ptr<RegexAST>(rhs.operand->clone());
        min = rhs.min;
        max = rhs.max;
    }
    [[nodiscard]] RegexASTMultiplication* clone () const override {
        return new RegexASTMultiplication (*this);
    }
    void set_possible_inputs_to_true (bool is_possible_input[]) const override {
        operand->set_possible_inputs_to_true(is_possible_input);
    }
    void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) override {
        operand->remove_delimiters_from_wildcard(delimiters);
    }
};


class RegexASTGroup : public RegexAST {
public:
    static uint32_t const UNICODE_MAX = 0x10FFFF;

    typedef std::pair<uint32_t, uint32_t> Range;
    
    RegexASTGroup();
    RegexASTGroup(RegexASTGroup* left, RegexASTLiteral* right);
    RegexASTGroup(RegexASTGroup* left, RegexASTGroup* right);
    explicit RegexASTGroup(RegexASTLiteral* right);
    explicit RegexASTGroup(RegexASTGroup* right);
    RegexASTGroup(RegexASTLiteral* left, RegexASTLiteral* right);
    RegexASTGroup(uint32_t min, uint32_t max);
    explicit RegexASTGroup(const std::vector<uint32_t>& literals);
    void add_state(RegexNFA* nfa, RegexNFA::State* end_state) override;
    void add_range(uint32_t min, uint32_t max);
    void add_literal(uint32_t literal);
    [[nodiscard]] RegexASTGroup* clone () const override {
        return new RegexASTGroup (*this);
    }
    void set_possible_inputs_to_true (bool is_possible_input[]) const override {
        if (!negate) {
            for (Range range: ranges) {
                for (uint32_t i = range.first; i <= range.second; i++) {
                    is_possible_input[i] = true;
                }
            }
        } else {
            std::vector<char> inputs (UNICODE_MAX, true);
            for (Range range: ranges) {
                for (uint32_t i = range.first; i <= range.second; i++) {
                    inputs[i] = false;
                }
            }
            for (uint32_t i = 0; i < inputs.size(); i++) {
                if (inputs[i]) {
                    is_possible_input[i] = true;
                }
            }
        }
    }
    void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) override {
        if(!is_wildcard) {
            return;
        }
        if(delimiters.empty()) {
            return;
        }
        ranges.clear();
        std::sort (delimiters.begin(), delimiters.end());
        if(delimiters[0] != 0) {
            Range range(0, delimiters[0] - 1);
            ranges.push_back(range);
        }
        for (uint32_t i = 1; i < delimiters.size(); i++) {
            if(delimiters[i] - delimiters[i-1] > 1) {
                Range range(delimiters[i - 1] + 1, delimiters[i] - 1);
                ranges.push_back(range);
            }
        }
        if(delimiters.back() != RegexASTGroup::UNICODE_MAX) {
            Range range(delimiters.back() + 1, RegexASTGroup::UNICODE_MAX);
            ranges.push_back(range);
        }
    }
    
    void set_is_wildcard_true() {
        is_wildcard = true;
    }

private:
    bool is_wildcard;
    bool negate;
    std::vector<Range> ranges;

    static std::vector<Range> merge(const std::vector<Range>& ranges);
    static std::vector<Range> complement(const std::vector<Range>& ranges);
};

/*
class RegexASTChain : public RegexAST {
public:
    std::vector<std::unique_ptr<RegexAST>> sequence;

    RegexASTChain(std::vector<std::unique_ptr<RegexAST>>);
};
*/

#endif // FINITEAUTOMATA_HPP
