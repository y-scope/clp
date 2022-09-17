#ifndef COMPRESSOR_FRONTEND_LALR1Parser_HPP
#define COMPRESSOR_FRONTEND_LALR1Parser_HPP

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
    template<class... Ts>
    struct overloaded : Ts ... {
        using Ts::operator()...;
    }; // (1)
    template<class... Ts> overloaded (Ts...) -> overloaded<Ts...>;  // (2)

    class NonTerminal {
    public:
        static MatchedSymbol all_children[];
        static uint32_t next_children_start;
        Production* production;
        std::unique_ptr<ParserAST> value;

        uint32_t children_start;

        NonTerminal () : production(nullptr), children_start(0), value(nullptr) {}

        explicit NonTerminal (Production*);

        [[nodiscard]] Token* token_cast (int i) const {
            return &std::get<Token>(NonTerminal::all_children[children_start + i]);
        }

        [[nodiscard]] NonTerminal* nonterminal_cast (int i) const {
            return &std::get<NonTerminal>(NonTerminal::all_children[children_start + i]);
        }

        std::unique_ptr<ParserAST>& getParserAST () {
            return value;
        }
    };

    struct Production {
    public:
        uint32_t index;
        uint32_t head;
        std::vector<uint32_t> body;
        SemanticRule semantic_rule;

        [[nodiscard]] bool is_epsilon () const {
            return this->body.empty();
        }
    };

    struct Item {
    public:
        Production* production;
        uint32_t dot;
        // for LR0 items, `lookahead` is unused
        uint32_t lookahead;

        Item () = default;

        Item (Production* p, uint32_t d, uint32_t t) : production(p), dot(d), lookahead(t) {
        }

        [[nodiscard]] bool has_dot_at_end () const {
            return this->dot == this->production->body.size();
        }

        [[nodiscard]] uint32_t next_symbol () const {
            return this->production->body.at(this->dot);
        }

        friend bool operator< (const Item& lhs, const Item& rhs) {
            return std::tie(lhs.production->index, lhs.dot, lhs.lookahead) <
                   std::tie(rhs.production->index, rhs.dot, rhs.lookahead);
        }
    };

    struct ItemSet {
    public:
        uint32_t index = -1;
        std::set<Item> kernel;
        std::set<Item> closure;
        std::unordered_map<uint32_t, ItemSet*> next;
        std::unordered_map<uint32_t, std::vector<Production*>> reductions;
        std::vector<Action> actions;

        friend bool operator< (const ItemSet& lhs, const ItemSet& rhs) {
            return lhs.kernel < rhs.kernel;
        }

        bool empty () const {
            return kernel.empty();
        }
    };

    /// TODO: make LALR1Parser an abstract class?
    class LALR1Parser {
    public:
        LALR1Parser ();

        void add_rule (const std::string& name, std::unique_ptr<RegexAST> rule);

        void add_token (const std::string& name, char rule_char);

        void add_token_group (const std::string& name, std::unique_ptr<RegexASTGroup> rule_group);

        void add_token_chain (const std::string& name, const std::string& chain);

        uint32_t add_production (const std::string& head, const std::vector<std::string>& body, SemanticRule semantic_rule);

        void generate ();

        NonTerminal parse (ReaderInterface& reader);
        
        void set_archive_writer_ptr (streaming_archive::writer::Archive* value) {
            archive_writer_ptr = value;
        }
        
        [[nodiscard]] streaming_archive::writer::Archive* get_archive_writer_ptr () const {
            return archive_writer_ptr;
        }
        
    protected:
        void reset (ReaderInterface& reader);

        std::string report_error (ReaderInterface& reader);

        Lexer m_lexer;
        streaming_archive::writer::Archive* archive_writer_ptr;
        std::stack<MatchedSymbol> m_parse_stack_matches;
        std::stack<ItemSet*> m_parse_stack_states;
        ItemSet* root_itemset_ptr;
        std::optional<Token> m_next_token;
        std::vector<std::unique_ptr<Production>> m_productions;
        std::unordered_map<std::string, std::map<std::vector<std::string>, Production*>> m_productions_map;
        std::unordered_map<uint32_t, std::vector<Production*>> m_nonterminals;
        uint32_t root_production_id;

    private:
        std::set<uint32_t> m_terminals;
        std::set<uint32_t> m_nullable;
        std::map<std::set<Item>, std::unique_ptr<ItemSet>> m_lr0_itemsets;
        std::map<std::set<Item>, std::unique_ptr<ItemSet>> m_lr1_itemsets;
        std::unordered_map<uint32_t, std::set<uint32_t>> m_firsts;
        std::unordered_map<Production*, std::set<uint32_t>> m_spontaneous_map;
        std::map<Item, std::set<Item>> m_propagate_map;
        std::unordered_map<uint32_t, std::map<uint32_t, uint32_t>> go_to_table;

        void generate_lr0_kernels ();

        void generate_lr0_closure (ItemSet*);

        bool lr_closure_helper (ItemSet* item_set_ptr, Item const* item, uint32_t* next_symbol);

        ItemSet* go_to (ItemSet*, const uint32_t&);

        void generate_first_sets ();

        void generate_lr1_itemsets ();

        void generate_lr1_closure (ItemSet* item_set_ptr);

        void generate_lalr1_parsing_table ();

        void generate_lalr1_goto ();

        void generate_lalr1_action ();

        Token get_next_symbol ();

        bool parse_advance (Token& next_symbol, bool* accept);

        bool parse_symbol (uint32_t const& terminal, Token& next_symbol, bool* accept);

        static std::string get_input_after_last_newline (std::stack<MatchedSymbol>& parse_stack_matches);

        std::string get_input_until_next_newline (ReaderInterface& reader, Token* error_token);

        bool symbol_is_token (uint32_t s) {
            return m_terminals.find(s) != m_terminals.end();
        }
    };
}

#endif // COMPRESSOR_FRONTEND_LALR1Parser_HPP
