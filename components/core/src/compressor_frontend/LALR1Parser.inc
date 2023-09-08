#ifndef COMPRESSOR_FRONTEND_LALR1_PARSER_TPP
#define COMPRESSOR_FRONTEND_LALR1_PARSER_TPP

#include "LALR1Parser.hpp"

// C++ standard libraries
#include <functional>
#include <iostream>

// Boost libraries
#include<boost/filesystem.hpp>

// Project headers
#include "../FileReader.hpp"
#include "../streaming_archive/writer/Archive.hpp"

using compressor_frontend::finite_automata::RegexAST;
using compressor_frontend::finite_automata::RegexASTCat;
using compressor_frontend::finite_automata::RegexASTGroup;
using compressor_frontend::finite_automata::RegexASTInteger;
using compressor_frontend::finite_automata::RegexASTLiteral;
using compressor_frontend::finite_automata::RegexASTMultiplication;
using compressor_frontend::finite_automata::RegexASTOr;
using std::cout;
using std::deque;
using std::holds_alternative;
using std::make_unique;
using std::map;
using std::pair;
using std::set;
using std::string;
using std::unique_ptr;
using std::vector;

namespace compressor_frontend {
    template <typename NFAStateType, typename DFAStateType>
    LALR1Parser<NFAStateType, DFAStateType>::LALR1Parser () : m_archive_writer_ptr(nullptr), root_itemset_ptr(nullptr), m_root_production_id(0) {
        m_lexer.m_symbol_id[cTokenEnd] = (int) SymbolID::TokenEndID;
        m_lexer.m_symbol_id[cTokenUncaughtString] = (int) SymbolID::TokenUncaughtStringID;
        m_lexer.m_symbol_id[cTokenInt] = (int) SymbolID::TokenIntId;
        m_lexer.m_symbol_id[cTokenFloat] = (int) SymbolID::TokenFloatId;
        m_lexer.m_symbol_id[cTokenFirstTimestamp] = (int) SymbolID::TokenFirstTimestampId;
        m_lexer.m_symbol_id[cTokenNewlineTimestamp] = (int) SymbolID::TokenNewlineTimestampId;
        m_lexer.m_symbol_id[cTokenNewline] = (int) SymbolID::TokenNewlineId;

        m_lexer.m_id_symbol[(int) SymbolID::TokenEndID] = cTokenEnd;
        m_lexer.m_id_symbol[(int) SymbolID::TokenUncaughtStringID] = cTokenUncaughtString;
        m_lexer.m_id_symbol[(int) SymbolID::TokenIntId] = cTokenInt;
        m_lexer.m_id_symbol[(int) SymbolID::TokenFloatId] = cTokenFloat;
        m_lexer.m_id_symbol[(int) SymbolID::TokenFirstTimestampId] = cTokenFirstTimestamp;
        m_lexer.m_id_symbol[(int) SymbolID::TokenNewlineTimestampId] = cTokenNewlineTimestamp;
        m_lexer.m_id_symbol[(int) SymbolID::TokenNewlineId] = cTokenNewline;

        m_terminals.insert((int) SymbolID::TokenEndID);
        m_terminals.insert((int) SymbolID::TokenUncaughtStringID);
        m_terminals.insert((int) SymbolID::TokenIntId);
        m_terminals.insert((int) SymbolID::TokenFloatId);
        m_terminals.insert((int) SymbolID::TokenFirstTimestampId);
        m_terminals.insert((int) SymbolID::TokenNewlineTimestampId);
        m_terminals.insert((int) SymbolID::TokenNewlineId);
    }


    template <typename NFAStateType, typename DFAStateType>
    void LALR1Parser<NFAStateType, DFAStateType>::add_rule (const string& name, unique_ptr<RegexAST<NFAStateType>> rule) {
        if (m_lexer.m_symbol_id.find(name) == m_lexer.m_symbol_id.end()) {
            m_lexer.m_symbol_id[name] = m_lexer.m_symbol_id.size();
            m_lexer.m_id_symbol[m_lexer.m_symbol_id[name]] = name;

        }
        m_lexer.add_rule(m_lexer.m_symbol_id[name], std::move(rule));
        m_terminals.insert(m_lexer.m_symbol_id[name]);
    }

    template <typename NFAStateType, typename DFAStateType>
    void LALR1Parser<NFAStateType, DFAStateType>::add_token (const string& name, char rule_char) {
        add_rule(name, make_unique<RegexASTLiteral<NFAStateType>>(RegexASTLiteral<NFAStateType>(rule_char)));
    }

    template <typename NFAStateType, typename DFAStateType>
    void LALR1Parser<NFAStateType, DFAStateType>::add_token_group (const string& name, unique_ptr<RegexASTGroup<NFAStateType>> rule_group) {
        add_rule(name, std::move(rule_group));
    }

    template <typename NFAStateType, typename DFAStateType>
    void LALR1Parser<NFAStateType, DFAStateType>::add_token_chain (const string& name, const string& chain) {
        assert(chain.size() > 1);
        unique_ptr<RegexASTLiteral<NFAStateType>> first_char_rule = make_unique<RegexASTLiteral<NFAStateType>>(RegexASTLiteral<NFAStateType>(chain[0]));
        unique_ptr<RegexASTLiteral<NFAStateType>> second_char_rule = make_unique<RegexASTLiteral<NFAStateType>>(RegexASTLiteral<NFAStateType>(chain[1]));
        unique_ptr<RegexASTCat<NFAStateType>> rule_chain = make_unique<RegexASTCat<NFAStateType>>(std::move(first_char_rule), std::move(second_char_rule));
        for (uint32_t i = 2; i < chain.size(); i++) {
            char next_char = chain[i];
            unique_ptr<RegexASTLiteral<NFAStateType>> next_char_rule = make_unique<RegexASTLiteral<NFAStateType>>(RegexASTLiteral<NFAStateType>(next_char));
            rule_chain = make_unique<RegexASTCat<NFAStateType>>(std::move(rule_chain), std::move(next_char_rule));
        }
        add_rule(name, std::move(rule_chain));
    }

    template <typename NFAStateType, typename DFAStateType>
    uint32_t LALR1Parser<NFAStateType, DFAStateType>::add_production (const string& head, const vector<string>& body, SemanticRule semantic_rule) {
        if (m_lexer.m_symbol_id.find(head) == m_lexer.m_symbol_id.end()) {
            m_lexer.m_symbol_id[head] = m_lexer.m_symbol_id.size();
            m_lexer.m_id_symbol[m_lexer.m_symbol_id[head]] = head;
        }
        uint32_t n = m_productions.size();
        auto it = m_productions_map.find(head);
        if (it != m_productions_map.end()) {
            map<vector<string>, Production*>::iterator it2;
            it2 = it->second.find(body);
            if (it2 != it->second.end()) {
                it2->second->m_semantic_rule = semantic_rule;
                return n;
            }
        }
        unique_ptr<Production> p(new Production);
        p->m_index = n;
        p->m_head = m_lexer.m_symbol_id[head];
        for (const string& symbol_string: body) {
            if (m_lexer.m_symbol_id.find(symbol_string) == m_lexer.m_symbol_id.end()) {
                m_lexer.m_symbol_id[symbol_string] = m_lexer.m_symbol_id.size();
                m_lexer.m_id_symbol[m_lexer.m_symbol_id[symbol_string]] = symbol_string;
            }
            p->m_body.push_back(m_lexer.m_symbol_id[symbol_string]);
        }
        p->m_semantic_rule = std::move(semantic_rule);
        m_nonterminals.insert(pair<int, vector<Production*>>(p->m_head, {}));
        m_nonterminals[p->m_head].push_back(p.get());
        m_productions_map[head][body] = p.get();
        m_productions.push_back(std::move(p));
        if (m_productions.size() == 1) {
            m_root_production_id = add_production("$START_PRIME", {head}, nullptr);
        }
        return n;
    }

    template <typename NFAStateType, typename DFAStateType>
    void LALR1Parser<NFAStateType, DFAStateType>::generate () {
        m_lexer.generate();
        assert(!m_productions.empty());
        generate_lr0_kernels();
        generate_first_sets();
        generate_lr1_itemsets();
        generate_lalr1_parsing_table();
    }

    template <typename NFAStateType, typename DFAStateType>
    void LALR1Parser<NFAStateType, DFAStateType>::generate_lr0_kernels () {
        Production* root_production_ptr = m_productions[m_root_production_id].get();
        Item root_item(root_production_ptr, 0, cNullSymbol);
        unique_ptr<ItemSet> item_set0 = make_unique<ItemSet>();
        item_set0->m_kernel.insert(root_item);
        deque<ItemSet*> unused_item_sets;
        item_set0->m_index = m_lr0_itemsets.size();
        unused_item_sets.push_back(item_set0.get());
        m_lr0_itemsets[item_set0->m_kernel] = std::move(item_set0);
        while (!unused_item_sets.empty()) {
            ItemSet* item_set_ptr = unused_item_sets.back();
            unused_item_sets.pop_back();
            generate_lr0_closure(item_set_ptr);
            for (const uint32_t& next_symbol: m_terminals) {
                ItemSet* new_item_set_ptr = go_to(item_set_ptr, next_symbol);
                if (new_item_set_ptr != nullptr) {
                    unused_item_sets.push_back(new_item_set_ptr);
                }
            }
            for (map<uint32_t, vector<Production*>>::value_type const& kv: m_nonterminals) {
                uint32_t next_symbol = kv.first;
                ItemSet* new_item_set_ptr = go_to(item_set_ptr, next_symbol);
                if (new_item_set_ptr != nullptr) {
                    unused_item_sets.push_back(new_item_set_ptr);
                }
            }
        }
    }

    template <typename NFAStateType, typename DFAStateType>
    bool LALR1Parser<NFAStateType, DFAStateType>::lr_closure_helper (ItemSet* item_set_ptr, const Item* item, uint32_t* next_symbol) {
        if (!item_set_ptr->m_closure.insert(*item).second) { // add {S'->(dot)S, ""}
            return true;
        }
        if (item->has_dot_at_end()) {
            return true;
        }
        *next_symbol = item->next_symbol();
        if (this->symbol_is_token(*next_symbol)) { // false
            return true;
        }
        return false;
    }

    template <typename NFAStateType, typename DFAStateType>
    void LALR1Parser<NFAStateType, DFAStateType>::generate_lr0_closure (ItemSet* item_set_ptr) {
        deque<Item> q(item_set_ptr->m_kernel.begin(), item_set_ptr->m_kernel.end()); // {{S'->(dot)S, ""}}
        while (!q.empty()) {
            Item item = q.back(); // {S'->(dot)S, ""}
            q.pop_back();
            uint32_t next_symbol;
            if (lr_closure_helper(item_set_ptr, &item, &next_symbol)) {
                continue;
            }
            if (m_nonterminals.find(next_symbol) == m_nonterminals.end()) {
                assert(false);
            }
            for (Production* const p: m_nonterminals.at(next_symbol)) { // S -> a
                q.emplace_back(p, 0, cNullSymbol); // {S -> (dot) a, ""}
            }
        }
    }

    template <typename NFAStateType, typename DFAStateType>
    ItemSet* LALR1Parser<NFAStateType, DFAStateType>::go_to (ItemSet* from_item_set, const uint32_t& next_symbol) {
        unique_ptr<ItemSet> next_item_set_ptr = make_unique<ItemSet>();
        assert(from_item_set != nullptr);
        for (Item const& item: from_item_set->m_closure) {
            if (item.has_dot_at_end()) {
                continue;
            }
            if (item.next_symbol() == next_symbol) {
                next_item_set_ptr->m_kernel.emplace(item.m_production, item.m_dot + 1, item.m_lookahead);
            }
        }
        if (next_item_set_ptr->m_kernel.empty()) {
            return nullptr;
        }
        if (m_lr0_itemsets.find(next_item_set_ptr->m_kernel) != m_lr0_itemsets.end()) {
            ItemSet* existing_item_set_ptr = m_lr0_itemsets[next_item_set_ptr->m_kernel].get();
            m_go_to_table[from_item_set->m_index][next_symbol] = existing_item_set_ptr->m_index;
            from_item_set->m_next[next_symbol] = existing_item_set_ptr;
        } else {
            next_item_set_ptr->m_index = m_lr0_itemsets.size();
            m_go_to_table[from_item_set->m_index][next_symbol] = next_item_set_ptr->m_index;
            from_item_set->m_next[next_symbol] = next_item_set_ptr.get();
            m_lr0_itemsets[next_item_set_ptr->m_kernel] = std::move(next_item_set_ptr);
            return from_item_set->m_next[next_symbol];
        }
        return nullptr;
    }

    template <typename NFAStateType, typename DFAStateType>
    void LALR1Parser<NFAStateType, DFAStateType>::generate_first_sets () {
        for (uint32_t const& s: m_terminals) {
            m_firsts.insert(pair<uint32_t, set<uint32_t>>(s, {s}));
        }
        bool changed = true;
        while (changed) {
            changed = false;
            for (const unique_ptr<Production>& p: m_productions) {
                set<uint32_t>& f = m_firsts[p->m_head];
                if (p->is_epsilon()) {
                    changed = changed || m_nullable.insert(p->m_head).second;
                    continue;
                }
                size_t old = f.size();
                size_t i = 0;
                for (uint32_t const& s: p->m_body) {
                    set<uint32_t>& f2 = m_firsts[s];
                    f.insert(f2.begin(), f2.end());
                    if (m_nullable.find(s) == m_nullable.end()) {
                        break;
                    }
                    i++;
                }
                if (i == p->m_body.size()) {
                    changed = changed || m_nullable.insert(p->m_head).second;
                }
                changed = changed || (f.size() != old);
            }
        }
    }

    template <typename NFAStateType, typename DFAStateType>
    void LALR1Parser<NFAStateType, DFAStateType>::generate_lr1_itemsets () {
        for (map<set<Item>, unique_ptr<ItemSet>>::value_type const& kv: m_lr0_itemsets) {
            for (Item const& l0_item: kv.second->m_kernel) {
                ItemSet temp_item_set;
                temp_item_set.m_kernel.insert(l0_item);
                generate_lr1_closure(&temp_item_set);
                for (Item const& l1_item: temp_item_set.m_closure) {
                    if (l1_item.m_lookahead != cNullSymbol) {
                        m_spontaneous_map[l1_item.m_production].insert(l1_item.m_lookahead);
                    } else {
                        if (l1_item.m_dot < l1_item.m_production->m_body.size()) {
                            Item temp_item(l1_item.m_production, l1_item.m_dot + 1, cNullSymbol);
                            m_propagate_map[l0_item].insert(temp_item);
                        }
                    }
                }
            }
        }
        map<Item, set<int>> lookaheads;
        for (map<set<Item>, unique_ptr<ItemSet>>::value_type const& kv: m_lr0_itemsets) {
            for (Item const& l0_item: kv.second->m_kernel) {
                lookaheads[l0_item].insert(m_spontaneous_map[l0_item.m_production].begin(),
                                           m_spontaneous_map[l0_item.m_production].end());
                if (l0_item.m_production == m_productions[m_root_production_id].get()) {
                    lookaheads[l0_item].insert((int) SymbolID::TokenEndID);
                }
            }
        }
        bool changed = true;
        while (changed) {
            changed = false;
            for (map<Item, set<Item>>::value_type& kv: m_propagate_map) {
                Item item_from = kv.first;
                for (Item const& item_to: kv.second) {
                    size_t size_before = lookaheads[item_to].size();
                    lookaheads[item_to].insert(lookaheads[item_from].begin(), lookaheads[item_from].end());
                    size_t size_after = lookaheads[item_to].size();
                    changed = changed || size_after > size_before;
                }
            }
        }
        for (map<set<Item>, unique_ptr<ItemSet>>::value_type const& kv: m_lr0_itemsets) {
            unique_ptr<ItemSet> lr1_item_set_ptr = make_unique<ItemSet>();
            for (Item const& l0_item: kv.second->m_kernel) {
                for (int const& lookahead: lookaheads[l0_item]) {
                    Item lr1_item(l0_item.m_production, l0_item.m_dot, lookahead);
                    lr1_item_set_ptr->m_kernel.insert(lr1_item);
                }
                if (l0_item.m_production == m_productions[m_root_production_id].get() && l0_item.m_dot == 0) {
                    root_itemset_ptr = lr1_item_set_ptr.get();
                }
            }
            generate_lr1_closure(lr1_item_set_ptr.get());
            lr1_item_set_ptr->m_index = kv.second->m_index;
            m_lr1_itemsets[lr1_item_set_ptr->m_kernel] = std::move(lr1_item_set_ptr);
        }
        // this seems like the wrong way to do this still:
        for (map<set<Item>, unique_ptr<ItemSet>>::value_type const& kv1: m_lr1_itemsets) {
            for (map<int, int>::value_type next_index: m_go_to_table[kv1.second->m_index]) {
                bool success = false;
                for (map<set<Item>, unique_ptr<ItemSet>>::value_type const& kv2: m_lr1_itemsets) {
                    if (next_index.second == kv2.second->m_index) {
                        kv1.second->m_next[next_index.first] = kv2.second.get();
                        success = true;
                        break;
                    }
                }
                assert(success);
            }
        }
    }

    template <typename NFAStateType, typename DFAStateType>
    void LALR1Parser<NFAStateType, DFAStateType>::generate_lr1_closure (ItemSet* item_set_ptr) {
        deque<Item> queue(item_set_ptr->m_kernel.begin(), item_set_ptr->m_kernel.end());
        while (!queue.empty()) {
            Item item = queue.back();
            queue.pop_back();
            uint32_t next_symbol;
            if (lr_closure_helper(item_set_ptr, &item, &next_symbol)) {
                continue;
            }
            vector<uint32_t> lookaheads;
            size_t pos = item.m_dot + 1;
            while (pos < item.m_production->m_body.size()) {
                uint32_t symbol = item.m_production->m_body.at(pos);
                set<uint32_t> symbol_firsts = m_firsts.find(symbol)->second;
                lookaheads.insert(lookaheads.end(), std::make_move_iterator(symbol_firsts.begin()),
                                  std::make_move_iterator(symbol_firsts.end()));
                if (m_nullable.find(symbol) == m_nullable.end()) {
                    break;
                }
                pos++;
            }
            if (pos == item.m_production->m_body.size()) {
                lookaheads.push_back(item.m_lookahead);
            }
            for (Production* const p: m_nonterminals.at(next_symbol)) {
                for (uint32_t const& l: lookaheads) {
                    queue.emplace_back(p, 0, l);
                }
            }
        }
    }

    template <typename NFAStateType, typename DFAStateType>
    void LALR1Parser<NFAStateType, DFAStateType>::generate_lalr1_parsing_table () {
        generate_lalr1_goto();
        generate_lalr1_action();
    }

    template <typename NFAStateType, typename DFAStateType>
    void LALR1Parser<NFAStateType, DFAStateType>::generate_lalr1_goto () {
        // done already at end of generate_lr1_itemsets()?
    }

    // Dragon book page 253
    template <typename NFAStateType, typename DFAStateType>
    void LALR1Parser<NFAStateType, DFAStateType>::generate_lalr1_action () {
        for (map<set<Item>, unique_ptr<ItemSet>>::value_type const& kv: m_lr1_itemsets) {
            ItemSet* item_set_ptr = kv.second.get();
            item_set_ptr->m_actions.resize(m_lexer.m_symbol_id.size(), false);
            for (Item const& item: item_set_ptr->m_closure) {
                if (!item.has_dot_at_end()) {
                    if (m_terminals.find(item.next_symbol()) == m_terminals.end() &&
                        m_nonterminals.find(item.next_symbol()) == m_nonterminals.end()) {
                        continue;
                    }
                    assert(item_set_ptr->m_next.find(item.next_symbol()) != item_set_ptr->m_next.end());
                    Action& action = item_set_ptr->m_actions[item.next_symbol()];
                    if (!holds_alternative<bool>(action)) {
                        if (holds_alternative<ItemSet*>(action) && std::get<ItemSet*>(action) == item_set_ptr->m_next[item.next_symbol()]) {
                            continue;
                        }
                        cout << "Warning: For symbol " << m_lexer.m_id_symbol[item.next_symbol()] << ", adding shift to "
                             << item_set_ptr->m_next[item.next_symbol()]->m_index << " causes ";
                        if (holds_alternative<ItemSet*>(action)) {
                            cout << "shift-shift conflict with shift to " << std::get<ItemSet*>(action)->m_index << std::endl;
                        } else {
                            cout << "shift-reduce conflict with reduction " << m_lexer.m_id_symbol[std::get<Production*>(action)->m_head]
                                      << "-> {";
                            for (uint32_t symbol: std::get<Production*>(action)->m_body) {
                                cout << m_lexer.m_id_symbol[symbol] << ",";
                            }
                            cout << "}" << std::endl;
                        }
                    }
                    item_set_ptr->m_actions[item.next_symbol()] = item_set_ptr->m_next[item.next_symbol()];
                }
                if (item.has_dot_at_end()) {
                    if (item.m_production == m_productions[m_root_production_id].get()) {
                        Action action = true;
                        item_set_ptr->m_actions[(int) SymbolID::TokenEndID] = action;
                    } else {
                        Action& action = item_set_ptr->m_actions[item.m_lookahead];
                        if (!holds_alternative<bool>(action)) {
                            cout << "Warning: For symbol " << m_lexer.m_id_symbol[item.m_lookahead]
                                 << ", adding reduction " << m_lexer.m_id_symbol[item.m_production->m_head] << "-> {";
                            for (uint32_t symbol: item.m_production->m_body) {
                                cout << m_lexer.m_id_symbol[symbol] << ",";
                            }
                            cout << "} causes ";
                            if (holds_alternative<ItemSet*>(action)) {
                                cout << "shift-reduce conflict with shift to " << std::get<ItemSet*>(action)->m_index << std::endl;
                            } else {
                                cout << "reduce-reduce conflict with reduction "
                                          << m_lexer.m_id_symbol[std::get<Production*>(action)->m_head]
                                          << "-> {";
                                for (uint32_t symbol: std::get<Production*>(action)->m_body) {
                                    cout << m_lexer.m_id_symbol[symbol] << ",";
                                }
                                cout << "}" << std::endl;
                            }
                        }
                        item_set_ptr->m_actions[item.m_lookahead] = item.m_production;
                    }
                }
            }
        }
    }

    static uint32_t get_line_num (MatchedSymbol& top_symbol) {
        uint32_t line_num = -1;
        std::stack<MatchedSymbol> symbols;
        symbols.push(std::move(top_symbol));
        while (line_num == -1) {
            assert(!symbols.empty());
            MatchedSymbol& curr_symbol = symbols.top();
            std::visit(overloaded{
                    [&line_num] (Token& token) {
                        line_num = token.m_line;
                    },
                    [&symbols] (NonTerminal& m) {
                        for (int i = 0; i < m.m_production->m_body.size(); i++) {
                            symbols.push(std::move(NonTerminal::m_all_children[m.m_children_start + i]));
                        }
                    }
            }, curr_symbol);
            symbols.pop();
        }
        return line_num;
    }

    template <typename NFAStateType, typename DFAStateType>
    string LALR1Parser<NFAStateType, DFAStateType>::get_input_after_last_newline (std::stack<MatchedSymbol>& parse_stack_matches) {
        string error_message_reversed;
        bool done = false;
        while (!parse_stack_matches.empty() && !done) {
            MatchedSymbol top_symbol = std::move(parse_stack_matches.top());
            parse_stack_matches.pop();
            std::visit(overloaded{
                    [&error_message_reversed, &done] (Token& token) {
                        if (token.get_string() == "\r" || token.get_string() == "\n") {
                            done = true;
                        } else {
                            // input is being read backwards, so reverse each token so that when the entire input is reversed
                            // each token is displayed correctly
                            string token_string = token.get_string();
                            std::reverse(token_string.begin(), token_string.end());
                            error_message_reversed += token_string;
                        }
                    },
                    [&parse_stack_matches] (NonTerminal& m) {
                        for (int i = 0; i < m.m_production->m_body.size(); i++) {
                            parse_stack_matches.push(std::move(NonTerminal::m_all_children[m.m_children_start + i]));
                        }
                    }
            }, top_symbol);
        }
        std::reverse(error_message_reversed.begin(), error_message_reversed.end());
        return error_message_reversed;
    }

    template <typename NFAStateType, typename DFAStateType>
    string LALR1Parser<NFAStateType, DFAStateType>::get_input_until_next_newline (ReaderInterface& reader, Token* error_token) {
        string rest_of_line;
        bool next_is_end_token = (error_token->m_type_ids->at(0) == (int) SymbolID::TokenEndID);
        bool next_has_newline = (error_token->get_string().find('\n') != string::npos) || (error_token->get_string().find('\r') != string::npos);
        while (!next_has_newline && !next_is_end_token) {
            Token token = get_next_symbol();
            next_has_newline = (token.get_string().find('\n') != string::npos) || (token.get_string().find('\r') != string::npos);
            if (!next_has_newline) {
                rest_of_line += token.get_string();
                next_is_end_token = (token.m_type_ids->at(0) == (int) SymbolID::TokenEndID);
            }
        }
        rest_of_line += "\n";
        return rest_of_line;
    }

    static string unescape (char const& c) {
        switch (c) {
            case '\t':
                return "\\t";
            case '\r':
                return "\\r";
            case '\n':
                return "\\n";
            case '\v':
                return "\\v";
            case '\f':
                return "\\f";
            default:
                return {c};
        }
    }

    template <typename NFAStateType, typename DFAStateType>
    string LALR1Parser<NFAStateType, DFAStateType>::report_error (ReaderInterface& reader) {
        assert(m_next_token == std::nullopt);
        assert(!m_parse_stack_matches.empty());
        MatchedSymbol top_symbol = std::move(m_parse_stack_matches.top());
        m_parse_stack_matches.pop();
        uint32_t line_num = get_line_num(top_symbol);
        Token token = std::get<Token>(top_symbol);
        string consumed_input = get_input_after_last_newline(m_parse_stack_matches);
        string error_type = "unknown error";
        string error_indicator;
        Token error_token = token;
        string rest_of_line = get_input_until_next_newline(reader, &error_token);
        for (uint32_t i = 0; i < consumed_input.size() + 10; i++) {
            error_indicator += " ";
        }
        error_indicator += "^\n";
        if (token.m_type_ids->at(0) == (int) SymbolID::TokenEndID && consumed_input.empty()) {
            error_type = "empty file";
            error_indicator = "^\n";
        } else {
            error_type = "expected ";
            for (uint32_t i = 0; i < m_parse_stack_states.top()->m_actions.size(); i++) {
                Action action = m_parse_stack_states.top()->m_actions[i];
                if (action.index() != 0) {
                    error_type += "'";
                    if (auto* regex_ast_literal = dynamic_cast<RegexASTLiteral<NFAStateType>*>(m_lexer.get_rule(i))) {
                        error_type += unescape(char(regex_ast_literal->get_character()));
                    } else {
                        error_type += m_lexer.m_id_symbol[i];
                    }
                    error_type += "',";
                }
            }
            error_type.pop_back();
            error_type += " before '" + unescape(token.get_string()[0]) + "' token";
        }
        string file_name = boost::filesystem::canonical((dynamic_cast<FileReader&>(reader)).get_path()).string();
        string error_string = file_name + ":" + std::to_string(line_num + 1) + ":"
                                   + std::to_string(consumed_input.size() + 1) + ": error: " + error_type + "\n";
        for (int i = 0; i < 10; i++) {
            error_string += " ";
        }
        error_string += consumed_input + error_token.get_string() + rest_of_line + error_indicator;
        return error_string;
    }

    template <typename NFAStateType, typename DFAStateType>
    NonTerminal LALR1Parser<NFAStateType, DFAStateType>::parse (ReaderInterface& reader) {
        reset(reader);
        m_parse_stack_states.push(root_itemset_ptr);
        bool accept = false;
        while (true) {
            Token next_terminal = get_next_symbol();
            if (parse_advance(next_terminal, &accept)) {
                break;
            }
        }
        if (!accept) {
            throw std::runtime_error(report_error(reader));
        }
        assert(!m_parse_stack_matches.empty());
        MatchedSymbol m = std::move(m_parse_stack_matches.top());
        m_parse_stack_matches.pop();
        assert(m_parse_stack_matches.empty());
        return std::move(std::get<NonTerminal>(m));
    }

    template <typename NFAStateType, typename DFAStateType>
    void LALR1Parser<NFAStateType, DFAStateType>::reset (ReaderInterface& reader) {
        m_next_token = std::nullopt;
        while (!m_parse_stack_states.empty()) {
            m_parse_stack_states.pop();
        }
        while (!m_parse_stack_matches.empty()) {
            m_parse_stack_matches.pop();
        }
        m_lexer.reset(reader);
    }

    template <typename NFAStateType, typename DFAStateType>
    Token LALR1Parser<NFAStateType, DFAStateType>::get_next_symbol () {
        if (m_next_token == std::nullopt) {
            Token token = m_lexer.scan();
            return token;
        }
        Token s = std::move(m_next_token.value());
        m_next_token = std::nullopt;
        return s;
    }

    template <typename NFAStateType, typename DFAStateType>
    bool LALR1Parser<NFAStateType, DFAStateType>::parse_advance (Token& next_token, bool* accept) {
        for (int const& type: *(next_token.m_type_ids)) {
            if (parse_symbol(type, next_token, accept)) {
                return (*accept);
            }
        }
        assert(*accept == false);
        // For error handling
        m_parse_stack_matches.push(std::move(next_token));
        return true;
    }

    template <typename NFAStateType, typename DFAStateType>
    bool LALR1Parser<NFAStateType, DFAStateType>::parse_symbol (uint32_t const& type_id, Token& next_token, bool* accept) {
        ItemSet* curr = m_parse_stack_states.top();
        Action& it = curr->m_actions[type_id];
        bool ret;
        std::visit(overloaded{
                [&ret, &accept] (bool is_accepting) {
                    if (!is_accepting) {
                        ret = false;
                        return;
                    }
                    *accept = true;
                    ret = true;
                    return;
                },
                [&ret, &next_token, this] (ItemSet* shift) {
                    m_parse_stack_states.push(shift);
                    m_parse_stack_matches.push(std::move(next_token));
                    ret = true;
                    return;
                },
                [&ret, &next_token, this] (Production* reduce) {
                    m_next_token = std::move(next_token);
                    NonTerminal matched_nonterminal(reduce);
                    size_t n = reduce->m_body.size();
                    for (size_t i = 0; i < n; i++) {
                        m_parse_stack_states.pop();
                        NonTerminal::m_all_children[matched_nonterminal.m_children_start + n - i - 1] = std::move(m_parse_stack_matches.top());
                        m_parse_stack_matches.pop();
                    }
                    if (reduce->m_semantic_rule != nullptr) {
                        m_lexer.set_reduce_pos(m_next_token->m_start_pos - 1);
                        matched_nonterminal.m_ast = reduce->m_semantic_rule(&matched_nonterminal);
                    }
                    ItemSet* curr = m_parse_stack_states.top();
                    Action const& it = curr->m_actions[matched_nonterminal.m_production->m_head];
                    m_parse_stack_states.push(std::get<ItemSet*>(it));
                    m_parse_stack_matches.push(std::move(matched_nonterminal));
                    ret = true;
                    return;
                }
        }, it);
        return ret;
    }
}

#endif //COMPRESSOR_FRONTEND_LALR1_PARSER_TPP
