#ifndef COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_AST_HPP
#define COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_AST_HPP

// C++ standard libraries
#include <algorithm>
#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <vector>

// Project headers
#include "../Constants.hpp"
#include "RegexNFA.hpp"
#include "UnicodeIntervalTree.hpp"

namespace compressor_frontend::finite_automata {

    template <typename NFAStateType>
    class RegexAST {
    public:
        // Destructor
        virtual ~RegexAST () = default;

        /**
         * Used for cloning a unique_pointer of base type RegexAST
         * @return RegexAST*
         */
        [[nodiscard]] virtual RegexAST* clone () const = 0;
        
        /**
         * Sets is_possible_input to specify which utf8 characters are allowed in a lexer rule
         * @param is_possible_input
         */
        virtual void set_possible_inputs_to_true (bool is_possible_input[]) const = 0;

        /**
         * transform '.' from any-character into any non-delimiter in a lexer rule
         * @param delimiters
         */
        virtual void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) = 0;
        
        /**
         * Add the needed RegexNFA::states to the passed in nfa to handle the current node before transitioning to a pre-tagged end_state
         * @param nfa
         * @param end_state
         */
        virtual void add (RegexNFA<NFAStateType>* nfa, NFAStateType* end_state) = 0;
    };

    // Leaf node
    template <typename NFAStateType>
    class RegexASTLiteral : public RegexAST<NFAStateType> {
    public:
        // Constructor
        explicit RegexASTLiteral (uint32_t character);
        
        /**
         * Used for cloning a unique_pointer of type RegexASTLiteral
         * @return RegexASTLiteral*
         */
        [[nodiscard]] RegexASTLiteral* clone () const override {
            return new RegexASTLiteral(*this);
        }
        
        /**
         * Sets is_possible_input to specify which utf8 characters are allowed in a lexer rule containing RegexASTLiteral at a leaf node in its AST
         * @param is_possible_input
         */
        void set_possible_inputs_to_true (bool is_possible_input[]) const override {
            is_possible_input[m_character] = true;
        }

        /**
         * Transforms '.' to to be any non-delimiter in a lexer rule, which does nothing as RegexASTLiteral is a leaf node that is not a RegexASTGroup
         * @param delimiters
         */
        void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) override {
            // DO NOTHING
        }
        
        /**
         * Add the needed RegexNFA::states to the passed in nfa to handle a RegexASTLiteral before transitioning to a pre-tagged end_state
         * @param nfa
         * @param end_state
         */
        void add (RegexNFA<NFAStateType>* nfa, NFAStateType* end_state) override;

        [[nodiscard]] const uint32_t& get_character () const {
            return m_character;
        }
        
    private:
        uint32_t m_character;

    };

    // Leaf node
    template <typename NFAStateType>
    class RegexASTInteger : public RegexAST<NFAStateType> {
    public:
        // Constructor
        explicit RegexASTInteger (uint32_t digit);

        // Constructor
        RegexASTInteger (RegexASTInteger* left, uint32_t digit);

        /**
         * Used for cloning a unique_pointer of type RegexASTInteger
         * @return RegexASTInteger*
         */
        [[nodiscard]] RegexASTInteger* clone () const override {
            return new RegexASTInteger(*this);
        }
        
        /**
         * Sets is_possible_input to specify which utf8 characters are allowed in a lexer rule containing RegexASTInteger at a leaf node in its AST
         * @param is_possible_input
         */
        void set_possible_inputs_to_true (bool is_possible_input[]) const override {
            for (uint32_t i: m_digits) {
                is_possible_input[i + '0'] = true;
            }
        }

        /**
         * Transforms '.' to to be any non-delimiter in a lexer rule, which does nothing as RegexASTInteger is a leaf node that is not a RegexASTGroup
         * @param delimiters
         */
        void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) override {
            // DO NOTHING
        }

        /**
         * Add the needed RegexNFA::states to the passed in nfa to handle a RegexASTInteger before transitioning to a pre-tagged end_state
         * @param nfa
         * @param end_state
         */
        void add (RegexNFA<NFAStateType>* nfa, NFAStateType* end_state) override;

        [[nodiscard]] const std::vector<uint32_t>& get_digits () const {
            return m_digits;
        }

        [[nodiscard]] const uint32_t& get_digit (uint32_t i) const {
            return m_digits[i];
        }
        
    private:
        std::vector<uint32_t> m_digits;
    };

    // Lead node
    template <typename NFAStateType>
    class RegexASTGroup : public RegexAST<NFAStateType> {
    public:

        typedef std::pair<uint32_t, uint32_t> Range;

        // constructor
        RegexASTGroup ();

        // constructor
        RegexASTGroup (RegexASTGroup* left, RegexASTLiteral<NFAStateType>* right);

        // constructor
        RegexASTGroup (RegexASTGroup* left, RegexASTGroup* right);

        // constructor
        explicit RegexASTGroup (RegexASTLiteral<NFAStateType>* right);

        // constructor
        explicit RegexASTGroup (RegexASTGroup* right);

        // constructor
        RegexASTGroup (RegexASTLiteral<NFAStateType>* left, RegexASTLiteral<NFAStateType>* right);

        // constructor
        RegexASTGroup (uint32_t min, uint32_t max);

        // constructor
        explicit RegexASTGroup (const std::vector<uint32_t>& literals);

        /**
         * Used for cloning a unique_pointer of type RegexASTGroup
         * @return RegexASTGroup*
         */
        [[nodiscard]] RegexASTGroup* clone () const override {
            return new RegexASTGroup(*this);
        }

        /**
         * Sets is_possible_input to specify which utf8 characters are allowed in a lexer rule containing RegexASTGroup at a leaf node in its AST
         * @param is_possible_input
         */
        void set_possible_inputs_to_true (bool is_possible_input[]) const override {
            if (!m_negate) {
                for (Range range: m_ranges) {
                    for (uint32_t i = range.first; i <= range.second; i++) {
                        is_possible_input[i] = true;
                    }
                }
            } else {
                std::vector<char> inputs(cUnicodeMax, true);
                for (Range range: m_ranges) {
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

        /**
         * Transforms '.' to to be any non-delimiter in a lexer rule if this RegexASTGroup node contains `.` (is a wildcard group)
         * @param delimiters
         */
        void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) override {
            if (!m_is_wildcard) {
                return;
            }
            if (delimiters.empty()) {
                return;
            }
            m_ranges.clear();
            std::sort(delimiters.begin(), delimiters.end());
            if (delimiters[0] != 0) {
                Range range(0, delimiters[0] - 1);
                m_ranges.push_back(range);
            }
            for (uint32_t i = 1; i < delimiters.size(); i++) {
                if (delimiters[i] - delimiters[i - 1] > 1) {
                    Range range(delimiters[i - 1] + 1, delimiters[i] - 1);
                    m_ranges.push_back(range);
                }
            }
            if (delimiters.back() != cUnicodeMax) {
                Range range(delimiters.back() + 1, cUnicodeMax);
                m_ranges.push_back(range);
            }
        }

        /**
         * Add the needed RegexNFA::states to the passed in nfa to handle a RegexASTGroup before transitioning to a pre-tagged end_state
         * @param nfa
         * @param end_state
         */
        void add (RegexNFA<NFAStateType>* nfa, NFAStateType* end_state) override;
        
        void add_range (uint32_t min, uint32_t max) {
            m_ranges.emplace_back(min, max);
        }

        void add_literal (uint32_t literal) {
            m_ranges.emplace_back(literal, literal);
        }

        void set_is_wildcard_true () {
            m_is_wildcard = true;
        }

    private:
        /**
         * Merges multiple ranges such that the resulting m_ranges is sorted and non-overlapping
         * @param ranges
         * @return std::vector<Range>
         */
        static std::vector<Range> merge (const std::vector<Range>& ranges);
        
        /**
         * Takes the compliment (in the cast of regex `^` at the start of a group) of multiple ranges such that m_ranges is sorted and non-overlapping
         * @param ranges
         * @return std::vector<Range>
         */
        static std::vector<Range> complement (const std::vector<Range>& ranges);

        bool m_is_wildcard;
        bool m_negate;
        std::vector<Range> m_ranges;


    };
    
    // Intermediate node

    template <typename NFAStateType>
    class RegexASTOr : public RegexAST<NFAStateType> {
    public:
        // Constructor
        RegexASTOr (std::unique_ptr<RegexAST<NFAStateType>>, std::unique_ptr<RegexAST<NFAStateType>>);

        // Constructor
        RegexASTOr (const RegexASTOr& rhs) {
            m_left = std::unique_ptr<RegexAST<NFAStateType>>(rhs.m_left->clone());
            m_right = std::unique_ptr<RegexAST<NFAStateType>>(rhs.m_right->clone());
        }
        
        /**
         * Used for cloning a unique_pointer of type RegexASTOr
         * @return RegexASTOr*
         */
        [[nodiscard]] RegexASTOr* clone () const override {
            return new RegexASTOr(*this);
        }

        /**
         * Sets is_possible_input to specify which utf8 characters are allowed in a lexer rule containing RegexASTOr at a leaf node in its AST
         * @param is_possible_input
         */
        void set_possible_inputs_to_true (bool is_possible_input[]) const override {
            m_left->set_possible_inputs_to_true(is_possible_input);
            m_right->set_possible_inputs_to_true(is_possible_input);
        }
        
        /**
         * Transforms '.' to to be any non-delimiter in a lexer rule if RegexASTGroup with `.` is a descendant of this RegexASTOr node
         * @param delimiters
         */
        void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) override {
            m_left->remove_delimiters_from_wildcard(delimiters);
            m_right->remove_delimiters_from_wildcard(delimiters);
        }
        
        /**
         * Add the needed RegexNFA::states to the passed in nfa to handle a RegexASTOr before transitioning to a pre-tagged end_state
         * @param nfa
         * @param end_state
         */
        void add (RegexNFA<NFAStateType>* nfa, NFAStateType* end_state) override;

    private:
        std::unique_ptr<RegexAST<NFAStateType>> m_left;
        std::unique_ptr<RegexAST<NFAStateType>> m_right;
    };

    // Intermediate node
    template <typename NFAStateType>
    class RegexASTCat : public RegexAST<NFAStateType> {
    public:
        // Constructor
        RegexASTCat (std::unique_ptr<RegexAST<NFAStateType>>, std::unique_ptr<RegexAST<NFAStateType>>);

        // Constructor
        RegexASTCat (const RegexASTCat& rhs) {
            m_left = std::unique_ptr<RegexAST<NFAStateType>>(rhs.m_left->clone());
            m_right = std::unique_ptr<RegexAST<NFAStateType>>(rhs.m_right->clone());
        }
        
        /**
         * Used for cloning a unique_pointer of type RegexASTCat
         * @return RegexASTCat*
         */
        [[nodiscard]] RegexASTCat* clone () const override {
            return new RegexASTCat(*this);
        }

        /**
         * Sets is_possible_input to specify which utf8 characters are allowed in a lexer rule containing RegexASTCat at a leaf node in its AST
         * @param is_possible_input
         */
        void set_possible_inputs_to_true (bool is_possible_input[]) const override {
            m_left->set_possible_inputs_to_true(is_possible_input);
            m_right->set_possible_inputs_to_true(is_possible_input);
        }
        
        /**
         * Transforms '.' to to be any non-delimiter in a lexer rule if RegexASTGroup with `.` is a descendant of this RegexASTCat node
         * @param delimiters
         */
        void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) override {
            m_left->remove_delimiters_from_wildcard(delimiters);
            m_right->remove_delimiters_from_wildcard(delimiters);
        }
        
        /**
         * Add the needed RegexNFA::states to the passed in nfa to handle a RegexASTCat before transitioning to a pre-tagged end_state
         * @param nfa
         * @param end_state
         */
        void add (RegexNFA<NFAStateType>* nfa, NFAStateType* end_state) override;

    private:
        std::unique_ptr<RegexAST<NFAStateType>> m_left;
        std::unique_ptr<RegexAST<NFAStateType>> m_right;
    };

    // Intermediate node
    template <typename NFAStateType>
    class RegexASTMultiplication : public RegexAST<NFAStateType> {
    public:
        // Constructor
        RegexASTMultiplication (std::unique_ptr<RegexAST<NFAStateType>>, uint32_t, uint32_t);
        
        // Constructor
        RegexASTMultiplication (const RegexASTMultiplication& rhs) {
            m_operand = std::unique_ptr<RegexAST<NFAStateType>>(rhs.m_operand->clone());
            m_min = rhs.m_min;
            m_max = rhs.m_max;
        }
        
        /**
         * Used for cloning a unique_pointer of type RegexASTMultiplication
         * @return RegexASTMultiplication*
         */
        [[nodiscard]] RegexASTMultiplication* clone () const override {
            return new RegexASTMultiplication(*this);
        }
        
        /**
         * Sets is_possible_input to specify which utf8 characters are allowed in a lexer rule containing RegexASTMultiplication at a leaf node in its AST
         * @param is_possible_input
         */
        void set_possible_inputs_to_true (bool is_possible_input[]) const override {
            m_operand->set_possible_inputs_to_true(is_possible_input);
        }
        
        /**
         * Transforms '.' to to be any non-delimiter in a lexer rule if RegexASTGroup with `.` is a descendant of this RegexASTMultiplication node
         * @param delimiters
         */
        void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) override {
            m_operand->remove_delimiters_from_wildcard(delimiters);
        }
        
        /**
         * Add the needed RegexNFA::states to the passed in nfa to handle a RegexASTMultiplication before transitioning to a pre-tagged end_state
         * @param nfa
         * @param end_state
         */
        void add (RegexNFA<NFAStateType>* nfa, NFAStateType* end_state) override;

        [[nodiscard]] bool is_infinite () const {
            return this->m_max == 0;
        }

    private:
        std::unique_ptr<RegexAST<NFAStateType>> m_operand;
        uint32_t m_min;
        uint32_t m_max;
    };
}

#include "RegexAST.tpp"

#endif // COMPRESSOR_FRONTEND_FINITE_AUTOMATA_REGEX_AST_HPP
