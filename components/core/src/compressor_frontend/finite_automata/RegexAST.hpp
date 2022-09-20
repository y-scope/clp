#ifndef COMPRESSOR_FRONTEND_FINITEAUTOMATA_REGEXAST_HPP
#define COMPRESSOR_FRONTEND_FINITEAUTOMATA_REGEXAST_HPP

// C++ standard libraries
#include <algorithm>
#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <vector>

// Project headers
#include "../Constants.hpp"
#include "RegexDFA.hpp"
#include "RegexNFA.hpp"
#include "UnicodeIntervalTree.hpp"

namespace compressor_frontend::finite_automata {
    class RegexAST {
    public:
        virtual void add_state (RegexNFA* nfa, RegexNFA::State* end_state) = 0;

        virtual ~RegexAST () = default;

        [[nodiscard]] virtual RegexAST* clone () const = 0;

        virtual void set_possible_inputs_to_true (bool is_possible_input[]) const = 0;

        virtual void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) = 0;
    };


    class RegexASTLiteral : public RegexAST {
    public:
        explicit RegexASTLiteral (uint32_t character);

        void add_state (RegexNFA* nfa, RegexNFA::State* end_state) override;

        [[nodiscard]] RegexASTLiteral* clone () const override {
            return new RegexASTLiteral(*this);
        }

        void set_possible_inputs_to_true (bool is_possible_input[]) const override {
            is_possible_input[m_character] = true;
        }

        void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) override {
            // DO NOTHING
        }
        
        [[nodiscard]] const uint32_t& get_character () const {
            return m_character;
        }
        
    private:
        uint32_t m_character;

    };

    class RegexASTInteger : public RegexAST {
    public:
        explicit RegexASTInteger (uint32_t digit);

        RegexASTInteger (RegexASTInteger* left, uint32_t digit);

        void add_state (RegexNFA* nfa, RegexNFA::State* end_state) override;

        [[nodiscard]] RegexASTInteger* clone () const override {
            return new RegexASTInteger(*this);
        }

        void set_possible_inputs_to_true (bool is_possible_input[]) const override {
            for (uint32_t i: m_digits) {
                is_possible_input[i + '0'] = true;
            }
        }

        void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) override {
            // DO NOTHING
        }
        
        [[nodiscard]] const std::vector<uint32_t>& get_digits () const {
            return m_digits;
        }

        [[nodiscard]] const uint32_t& get_digit (uint32_t i) const {
            return m_digits[i];
        }
        
    private:
        std::vector<uint32_t> m_digits;
    };

    class RegexASTOr : public RegexAST {
    public:

        RegexASTOr (std::unique_ptr<RegexAST>, std::unique_ptr<RegexAST>);

        void add_state (RegexNFA* nfa, RegexNFA::State* end_state) override;

        RegexASTOr (const RegexASTOr& rhs) {
            m_left = std::unique_ptr<RegexAST>(rhs.m_left->clone());
            m_right = std::unique_ptr<RegexAST>(rhs.m_right->clone());
        }

        [[nodiscard]] RegexASTOr* clone () const override {
            return new RegexASTOr(*this);
        }

        void set_possible_inputs_to_true (bool is_possible_input[]) const override {
            m_left->set_possible_inputs_to_true(is_possible_input);
            m_right->set_possible_inputs_to_true(is_possible_input);
        }

        void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) override {
            m_left->remove_delimiters_from_wildcard(delimiters);
            m_right->remove_delimiters_from_wildcard(delimiters);
        }
        
    private:
        std::unique_ptr<RegexAST> m_left;
        std::unique_ptr<RegexAST> m_right;
    };

    class RegexASTCat : public RegexAST {
    public:
        RegexASTCat (std::unique_ptr<RegexAST>, std::unique_ptr<RegexAST>);

        void add_state (RegexNFA* nfa, RegexNFA::State* end_state) override;

        RegexASTCat (const RegexASTCat& rhs) {
            m_left = std::unique_ptr<RegexAST>(rhs.m_left->clone());
            m_right = std::unique_ptr<RegexAST>(rhs.m_right->clone());
        }

        [[nodiscard]] RegexASTCat* clone () const override {
            return new RegexASTCat(*this);
        }

        void set_possible_inputs_to_true (bool is_possible_input[]) const override {
            m_left->set_possible_inputs_to_true(is_possible_input);
            m_right->set_possible_inputs_to_true(is_possible_input);
        }

        void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) override {
            m_left->remove_delimiters_from_wildcard(delimiters);
            m_right->remove_delimiters_from_wildcard(delimiters);
        }
        
    private:
        std::unique_ptr<RegexAST> m_left;
        std::unique_ptr<RegexAST> m_right;
    };

    class RegexASTMultiplication : public RegexAST {
    public:
        [[nodiscard]] bool is_infinite () const {
            return this->m_max == 0;
        }

        RegexASTMultiplication (std::unique_ptr<RegexAST>, uint32_t, uint32_t);

        void add_state (RegexNFA* nfa, RegexNFA::State* end_state) override;

        RegexASTMultiplication (const RegexASTMultiplication& rhs) {
            m_operand = std::unique_ptr<RegexAST>(rhs.m_operand->clone());
            m_min = rhs.m_min;
            m_max = rhs.m_max;
        }

        [[nodiscard]] RegexASTMultiplication* clone () const override {
            return new RegexASTMultiplication(*this);
        }

        void set_possible_inputs_to_true (bool is_possible_input[]) const override {
            m_operand->set_possible_inputs_to_true(is_possible_input);
        }

        void remove_delimiters_from_wildcard (std::vector<uint32_t>& delimiters) override {
            m_operand->remove_delimiters_from_wildcard(delimiters);
        }
        
    private:
        std::unique_ptr<RegexAST> m_operand;
        uint32_t m_min;
        uint32_t m_max;
    };


    class RegexASTGroup : public RegexAST {
    public:

        typedef std::pair<uint32_t, uint32_t> Range;

        RegexASTGroup ();

        RegexASTGroup (RegexASTGroup* left, RegexASTLiteral* right);

        RegexASTGroup (RegexASTGroup* left, RegexASTGroup* right);

        explicit RegexASTGroup (RegexASTLiteral* right);

        explicit RegexASTGroup (RegexASTGroup* right);

        RegexASTGroup (RegexASTLiteral* left, RegexASTLiteral* right);

        RegexASTGroup (uint32_t min, uint32_t max);

        explicit RegexASTGroup (const std::vector<uint32_t>& literals);

        void add_state (RegexNFA* nfa, RegexNFA::State* end_state) override;

        void add_range (uint32_t min, uint32_t max);

        void add_literal (uint32_t literal);

        [[nodiscard]] RegexASTGroup* clone () const override {
            return new RegexASTGroup(*this);
        }

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

        void set_is_wildcard_true () {
            m_is_wildcard = true;
        }

    private:
        static std::vector<Range> merge (const std::vector<Range>& ranges);

        static std::vector<Range> complement (const std::vector<Range>& ranges);
        
        bool m_is_wildcard;
        bool m_negate;
        std::vector<Range> m_ranges;


    };
}
#endif // COMPRESSOR_FRONTEND_FINITEAUTOMATA_REGEXAST_HPP
