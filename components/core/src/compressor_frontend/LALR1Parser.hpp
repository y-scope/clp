#ifndef COMPRESSOR_FRONTEND_LALR1_PARSER_HPP
#define COMPRESSOR_FRONTEND_LALR1_PARSER_HPP

// C++ standard libraries
#include <cstdint>
#include <cassert>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <stack>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

// Project headers
#include "../ReaderInterface.hpp"
#include "../type_utils.hpp"
#include "Lexer.hpp"

namespace streaming_archive::writer {
    class File;

    class Archive;
}

namespace compressor_frontend {

    class ParserAST;

    class NonTerminal;

    template<typename T>
    class ParserValue;

    struct Production;
    struct Item;
    struct ItemSet;

    typedef std::function<std::unique_ptr<ParserAST> (NonTerminal*)> SemanticRule;
    typedef std::variant<bool, ItemSet*, Production*> Action;

    class ParserAST {
    public:
        // Constructor
        virtual ~ParserAST () = 0;

        template<typename T>
        T& get () {
            // TODO: why does this compile?
            return static_cast<ParserValue<T>*>(this)->value;
        }
    };
    
    template<typename T>
    class ParserValue : public ParserAST {
    public:
        T value;

        explicit ParserValue (T v) : value(std::move(v)) {}
    };
    
    typedef std::variant<Token, NonTerminal> MatchedSymbol;

    class NonTerminal {
    public:
        // Constructor
        NonTerminal () : m_production(nullptr), m_children_start(0), m_ast(nullptr) {}

        // Constructor
        explicit NonTerminal (Production*);

        /**
         * Return the ith child's (body of production) MatchedSymbol as a Token.
         * Note: only children are needed (and stored) for performing semantic actions (for the AST)
         * @param i
         * @return Token*
         */
        [[nodiscard]] Token* token_cast (int i) const {
            return &std::get<Token>(NonTerminal::m_all_children[m_children_start + i]);
        }

        /**
         * Return the ith child's (body of production) MatchedSymbol as a NonTerminal.
         * Note: only children are needed (and stored) for performing semantic actions (for the AST)
         * @param i
         * @return NonTerminal*
         */
        [[nodiscard]] NonTerminal* nonterminal_cast (int i) const {
            return &std::get<NonTerminal>(NonTerminal::m_all_children[m_children_start + i]);
        }

        /**
         * Return the AST that relates this nonterminal's children together (based on the production/syntax-rule that was determined to have generated them)
         * @return std::unique_ptr<ParserAST>
         */
        std::unique_ptr<ParserAST>& getParserAST () {
            return m_ast;
        }

        static MatchedSymbol m_all_children[];
        static uint32_t m_next_children_start;
        uint32_t m_children_start;
        Production* m_production;
        std::unique_ptr<ParserAST> m_ast;
    };

    /**
     * Structure representing a production of the form "m_head -> {m_body}".
     * The code fragment to execute upon reducing "{m_body} -> m_head" is m_semantic_rule, which is purely a function of the MatchedSymbols for {m_body}.
     * m_index is the productions position in the parsers production vector.
     */
    struct Production {
    public:
        /**
         * Returns if the production is an epsilon production. An epsilon production has nothing on its LHS (i.e., HEAD -> {})
         * @return bool
         */
        [[nodiscard]] bool is_epsilon () const {
            return this->m_body.empty();
        }

        uint32_t m_index;
        uint32_t m_head;
        std::vector<uint32_t> m_body;
        SemanticRule m_semantic_rule;
    };

    /**
     * Structure representing an item in a LALR1 state.
     * An item (1) is associated with a m_production and a single m_lookahead which is an input symbol (character) that can follow the m_production,
     * and (2) tracks the current matching progress of its associated m_production, where everything exclusively to the left of m_dot is already matched.
     */
    struct Item {
    public:
        // Constructor
        Item () = default;

        // Constructor
        Item (Production* p, uint32_t d, uint32_t t) : m_production(p), m_dot(d), m_lookahead(t) {
        }

        /**
         * Comparison operator for tie-breakers (not 100% sure where this is used)
         * @param lhs
         * @param rhs
         * @return bool
         */
        friend bool operator< (const Item& lhs, const Item& rhs) {
            return std::tie(lhs.m_production->m_index, lhs.m_dot, lhs.m_lookahead) <
                   std::tie(rhs.m_production->m_index, rhs.m_dot, rhs.m_lookahead);
        }

        /**
         * Returns if the item has a dot at the end. This indicates the production associated with the item has already been fully matched.
         * @return bool
         */
        [[nodiscard]] bool has_dot_at_end () const {
            return this->m_dot == this->m_production->m_body.size();
        }

        /**
         * Returns the next unmatched symbol in the production based on the dot.
         * @return uint32_t
         */
        [[nodiscard]] uint32_t next_symbol () const {
            return this->m_production->m_body.at(this->m_dot);
        }

        Production* m_production;
        uint32_t m_dot;
        uint32_t m_lookahead; // for LR0 items, `m_lookahead` is unused
    };

    /**
     * Structure representing an LALR1 state, a collection of items.
     * The m_kernel is sufficient for fully representing the state, but m_closure is useful for computations.
     * m_next indicates what state (ItemSet) to transition to based on the symbol received from the lexer
     * m_actions is the action to perform based on the symbol received from the lexer.
     */
    struct ItemSet {
    public:
        /**
         * Comparison operator for tie-breakers (not 100% sure where this is used)
         * @param lhs
         * @param rhs
         * @return bool
         */
        friend bool operator< (const ItemSet& lhs, const ItemSet& rhs) {
            return lhs.m_kernel < rhs.m_kernel;
        }

        bool empty () const {
            return m_kernel.empty();
        }
        
        uint32_t m_index = -1;
        std::set<Item> m_kernel;
        std::set<Item> m_closure;
        std::unordered_map<uint32_t, ItemSet*> m_next;
        std::vector<Action> m_actions;
    };

    /// TODO: make LALR1Parser an abstract class?
    template <typename NFAStateType, typename DFAStateType>
    class LALR1Parser {
    public:
        // Constructor
        LALR1Parser ();

        /// TODO: combine all the add_* into add_rule
        /**
         * Add a lexical rule to m_lexer
         * @param name
         * @param rule
         */
        void add_rule (const std::string& name, std::unique_ptr<RegexAST<NFAStateType>> rule);

        /**
         * Constructs a RegexASTLiteral and call add_rule
         * @param name
         * @param rule_char
         */
        void add_token (const std::string& name, char rule_char);

        /**
         * Calls add_rule with the given RegexASTGroup
         * @param name
         * @param rule_char
         */
        void add_token_group (const std::string& name, std::unique_ptr<finite_automata::RegexASTGroup<NFAStateType>> rule_group);

        /**
         * Constructs a RegexASTCat and calls add_rule
         * @param name
         * @param chain
         */
        void add_token_chain (const std::string& name, const std::string& chain);

        /**
         * Adds productions (syntax rule) to the parser
         * @param head
         * @param body
         * @param semantic_rule
         * @return uint32_t
         */
        uint32_t add_production (const std::string& head, const std::vector<std::string>& body, SemanticRule semantic_rule);

        /**
         * Generate the LALR1 parser (use after all the lexical rules and productions have been added)
         */
        void generate ();

        /// TODO: add throws to function headers
        /**
         * Parse an input (e.g. file)
         * @param reader
         * @return Nonterminal
         */
        NonTerminal parse (ReaderInterface& reader);

        void set_archive_writer_ptr (streaming_archive::writer::Archive* value) {
            m_archive_writer_ptr = value;
        }

        [[nodiscard]] streaming_archive::writer::Archive* get_archive_writer_ptr () const {
            return m_archive_writer_ptr;
        }
        
    protected:
        /**
         * Reset the parser to start a new parsing (set state to root, reset buffers, reset vars tracking positions)
         * @param reader
         */
        void reset (ReaderInterface& reader);

        /**
         * Return an error string based on the current error state, matched_stack, and next_symbol in the parser
         * @param reader
         * @return std::string
         */
        std::string report_error (ReaderInterface& reader);

        Lexer<NFAStateType, DFAStateType> m_lexer;
        streaming_archive::writer::Archive* m_archive_writer_ptr;
        std::stack<MatchedSymbol> m_parse_stack_matches;
        std::stack<ItemSet*> m_parse_stack_states;
        ItemSet* root_itemset_ptr;
        std::optional<Token> m_next_token;
        std::vector<std::unique_ptr<Production>> m_productions;
        std::unordered_map<std::string, std::map<std::vector<std::string>, Production*>> m_productions_map;
        std::unordered_map<uint32_t, std::vector<Production*>> m_nonterminals;
        uint32_t m_root_production_id;

    private:
        // Parser generation

        /**
         * Generate LR0 kernels based on the productions in m_productions
         */
        void generate_lr0_kernels ();

        /**
         * Perform closure for the specified item_set based on its kernel
         * @param item_set
         */
        void generate_lr0_closure (ItemSet* item_set_ptr);

        /**
         * Helper function for doing the closure on a specified item set
         * @param item_set_ptr
         * @param item
         * @param next_symbol
         * @return bool
         */
        bool lr_closure_helper (ItemSet* item_set_ptr, Item const* item, uint32_t* next_symbol);

        /**
         * Return the next state (ItemSet) based on the current state (ItemSet) and input symbol
         * @return ItemSet*
         */
        ItemSet* go_to (ItemSet*, const uint32_t&);

        /**
         * Generate m_firsts, which specify for each symbol, all possible prefixes (I think?)
         */
        void generate_first_sets ();

        /**
         * Generate kernels for LR1 item sets based on LR0 item sets
         */
        void generate_lr1_itemsets ();

        /**
         * Generate closure for a specified LR1 item set
         * @param item_set_ptr
         */
        void generate_lr1_closure (ItemSet* item_set_ptr);

        /**
         * Generating parsing table and goto table for LALR1 parser based on state-symbol pair
         * generate_lalr1_goto() + generate_lalr1_action()
         */
        void generate_lalr1_parsing_table ();

        /**
         *  Generating the goto table for LARL1 parser specifying which state (ItemSet) to transition to based on state-symbol pair
         *  Does nothing (its already done in an earlier step)
         */
        void generate_lalr1_goto ();

        /**
         *  Generating the action table for LARL1 parser specifying which action to perform based on state-symbol pair
         */
        void generate_lalr1_action ();

        // Parser utilization

        /**
         * Use the previous symbol from the lexer if unused, otherwise request the next symbol from the lexer
         * @return Token
         */
        Token get_next_symbol ();

        /**
         * Tries all symbols in the language that the next token may be until the first non-error symbol is tried
         * @param next_token
         * @param accept
         * @return bool
         */
        bool parse_advance (Token& next_token, bool* accept);

        /**
         * Perform an action and state transition based on the current state (ItemSet) and the type_id (current symbol interpretation of the next_token)
         * @param type_id
         * @param next_token
         * @param accept
         * @return bool
         */
        bool parse_symbol (uint32_t const& type_id, Token& next_token, bool* accept);

        // Error handling

        /**
         * Get the current line up to the error symbol
         * @param parse_stack_matches
         * @return std::string
         */
        static std::string get_input_after_last_newline (std::stack<MatchedSymbol>& parse_stack_matches);

        /**
         * Get the current line after the error symbol
         * @param reader
         * @param error_token
         * @return std::string
         */
        std::string get_input_until_next_newline (ReaderInterface& reader, Token* error_token);

        bool symbol_is_token (uint32_t s) {
            return m_terminals.find(s) != m_terminals.end();
        }

        // Variables
        std::set<uint32_t> m_terminals;
        std::set<uint32_t> m_nullable;
        std::map<std::set<Item>, std::unique_ptr<ItemSet>> m_lr0_itemsets;
        std::map<std::set<Item>, std::unique_ptr<ItemSet>> m_lr1_itemsets;
        std::unordered_map<uint32_t, std::set<uint32_t>> m_firsts;
        std::unordered_map<Production*, std::set<uint32_t>> m_spontaneous_map;
        std::map<Item, std::set<Item>> m_propagate_map;
        std::unordered_map<uint32_t, std::map<uint32_t, uint32_t>> m_go_to_table;
    };
}

#include "LALR1Parser.tpp"

#endif // COMPRESSOR_FRONTEND_LALR1_PARSER_HPP
