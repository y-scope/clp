#include <functional>
#include <iostream>

#include<boost/filesystem.hpp>

#include "LALR1Parser.hpp"
#include "LogParser.hpp"
#include "../FileReader.hpp"
#include "../streaming_archive/writer/Archive.hpp"

// std::string const LALR1Parser::NULL_SYMBOL = "NULL_SYMBOL";
const int LALR1Parser::NULL_SYMBOL = 10000000;
const uint32_t LALR1Parser::SIZE_OF_ALL_CHILDREN = 10000;
double LALR1Parser::MAX_MEMORY_USAGE = 0;
MatchedSymbol NonTerminal::all_children[LALR1Parser::SIZE_OF_ALL_CHILDREN];

ParserAST::~ParserAST() = default;

uint32_t NonTerminal::next_children_start = 0;
NonTerminal::NonTerminal(Production* p) : production(p), value(nullptr) {
    children_start = NonTerminal::next_children_start;
    NonTerminal::next_children_start += p->body.size();
}

void LALR1Parser::add_rule(std::string const& name, std::unique_ptr<RegexAST> rule) {
    if ( m_lexer.symbol_id.find(name) ==  m_lexer.symbol_id.end()) {
        m_lexer.symbol_id[name] = m_lexer.symbol_id.size();
        m_lexer.id_symbol[m_lexer.symbol_id[name]] = name;

    }
    m_lexer.add_rule(m_lexer.symbol_id[name], std::move(rule));
    m_terminals.insert(m_lexer.symbol_id[name]);
}

void LALR1Parser::add_token(std::string const& name, char rule_char) {
    add_rule(name, std::make_unique<RegexASTLiteral>(RegexASTLiteral(rule_char)));
}

void LALR1Parser::add_token_group(std::string const& name, std::unique_ptr<RegexASTGroup> rule_group) {
    add_rule(name, std::move(rule_group));
}

void LALR1Parser::add_token_chain(std::string const& name, std::string const& chain) {
    assert(chain.size() > 1);
    std::unique_ptr<RegexASTLiteral> first_char_rule = std::make_unique<RegexASTLiteral>(RegexASTLiteral(chain[0]));
    std::unique_ptr<RegexASTLiteral> second_char_rule = std::make_unique<RegexASTLiteral>(RegexASTLiteral(chain[1]));
    std::unique_ptr<RegexASTCat> rule_chain = std::make_unique<RegexASTCat>(std::move(first_char_rule), std::move(second_char_rule));
    for (uint32_t i = 2; i < chain.size(); i++) {
        char next_char = chain[i];
        std::unique_ptr<RegexASTLiteral> next_char_rule = std::make_unique<RegexASTLiteral>(RegexASTLiteral(next_char));
        rule_chain = std::make_unique<RegexASTCat>(std::move(rule_chain), std::move(next_char_rule));
    }
    add_rule(name, std::move(rule_chain));
}

uint32_t LALR1Parser::add_production(std::string const& head, const std::vector<std::string>& body,
                                 SemanticRule semantic_rule) {
    if (m_lexer.symbol_id.find(head) == m_lexer.symbol_id.end()) {
        m_lexer.symbol_id[head] = m_lexer.symbol_id.size();
        m_lexer.id_symbol[m_lexer.symbol_id[head]] = head;
    }
    uint32_t n = m_productions.size();
    auto it = m_productions_map.find(head);
    if (it != m_productions_map.end()) {
        std::map<std::vector<std::string>, Production *>::iterator it2;
        it2 = it->second.find(body);
        if (it2 != it->second.end()) {
            it2->second->semantic_rule = semantic_rule;
            return n;
        }
    }
    std::unique_ptr<Production> p(new Production);
    p->index = n;
    p->head = m_lexer.symbol_id[head];
    for (const std::string& symbol_string : body) {
        if (m_lexer.symbol_id.find(symbol_string) == m_lexer.symbol_id.end()) {
            m_lexer.symbol_id[symbol_string] = m_lexer.symbol_id.size();
            m_lexer.id_symbol[m_lexer.symbol_id[symbol_string]] = symbol_string;
        }
        p->body.push_back(m_lexer.symbol_id[symbol_string]);
    }
    p->semantic_rule = std::move(semantic_rule);
    m_nonterminals.insert(std::pair<int,std::vector<Production*>>(p->head,{}));
    m_nonterminals[p->head].push_back(p.get());
    m_productions_map[head][body] = p.get();
    m_productions.push_back(std::move(p));
    if (m_productions.size() == 1) {
        root_production_id = add_production("$START_PRIME", {head}, nullptr);
    }
    return n;
}

LALR1Parser::LALR1Parser() : archive_writer(nullptr), root_itemset_ptr(nullptr), root_production_id(0) {
    m_lexer.symbol_id[Lexer::TOKEN_END] = m_lexer.symbol_id.size();
    m_lexer.symbol_id[Lexer::TOKEN_UNCAUGHT_STRING] = m_lexer.symbol_id.size();
    m_lexer.id_symbol[Lexer::TOKEN_END_ID] = Lexer::TOKEN_END;
    m_lexer.id_symbol[Lexer::TOKEN_UNCAUGHT_STRING_ID] = Lexer::TOKEN_UNCAUGHT_STRING;
    assert(m_lexer.symbol_id[Lexer::TOKEN_END] == Lexer::TOKEN_END_ID);
    assert(m_lexer.symbol_id[Lexer::TOKEN_UNCAUGHT_STRING] == Lexer::TOKEN_UNCAUGHT_STRING_ID);
    m_terminals.insert(Lexer::TOKEN_END_ID);
    m_terminals.insert(Lexer::TOKEN_UNCAUGHT_STRING_ID);
}

void LALR1Parser::generate() {
    m_lexer.generate();
    assert(!m_productions.empty());
    generate_lr0_kernels();
    generate_first_sets();
    generate_lr1_itemsets();
    generate_lalr1_parsing_table();
}

void LALR1Parser::generate_lr0_kernels() {
    Production* root_production_ptr = m_productions[root_production_id].get();
    Item root_item(root_production_ptr, 0, LALR1Parser::NULL_SYMBOL);
    std::unique_ptr<ItemSet> item_set0 = std::make_unique<ItemSet>();
    item_set0->kernel.insert(root_item);
    std::deque<ItemSet*> unused_item_sets;
    item_set0->index = m_lr0_itemsets.size();
    unused_item_sets.push_back(item_set0.get());
    m_lr0_itemsets[item_set0->kernel] = std::move(item_set0);
    while (!unused_item_sets.empty()) {
        ItemSet* item_set_ptr = unused_item_sets.back();
        unused_item_sets.pop_back();
        generate_lr0_closure(item_set_ptr);
        for (uint32_t const& next_symbol : m_terminals) {
            ItemSet* new_item_set_ptr = go_to(item_set_ptr, next_symbol);
            if (new_item_set_ptr != nullptr) {
                unused_item_sets.push_back(new_item_set_ptr);
            }
        }
        for (std::map<uint32_t, std::vector<Production*>>::value_type const& kv : m_nonterminals) {
            uint32_t next_symbol = kv.first;
            ItemSet* new_item_set_ptr = go_to(item_set_ptr, next_symbol);
            if (new_item_set_ptr != nullptr) {
                unused_item_sets.push_back(new_item_set_ptr);
            }
        }
    }
}

bool LALR1Parser::lr_closure_helper(ItemSet* item_set_ptr, Item const* item, uint32_t* next_symbol) {
    if (!item_set_ptr->closure.insert(*item).second) { // add {S'->(dot)S, ""}
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

void LALR1Parser::generate_lr0_closure(ItemSet* item_set_ptr) {
    std::deque<Item> q(item_set_ptr->kernel.begin(), item_set_ptr->kernel.end()); // {{S'->(dot)S, ""}}
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
        for (Production* const p : m_nonterminals.at(next_symbol)) { // S -> a
            q.emplace_back(p, 0, LALR1Parser::NULL_SYMBOL); // {S -> (dot) a, ""}
        }
    }
}

ItemSet* LALR1Parser::go_to(ItemSet* from_item_set, const uint32_t& next_symbol) {
    std::unique_ptr<ItemSet> next_item_set_ptr = std::make_unique<ItemSet>();
    assert(from_item_set != nullptr);
    for (Item const& item : from_item_set->closure) {
        if (item.has_dot_at_end()) {
            continue;
        }
        if (item.next_symbol() == next_symbol) {
            next_item_set_ptr->kernel.emplace(item.production, item.dot + 1, item.lookahead);
        }
    }
    if (next_item_set_ptr->kernel.empty()) {
        return nullptr;
    }
    if (m_lr0_itemsets.find(next_item_set_ptr->kernel) != m_lr0_itemsets.end()) {
        ItemSet* existing_item_set_ptr = m_lr0_itemsets[next_item_set_ptr->kernel].get();
        go_to_table[from_item_set->index][next_symbol] = existing_item_set_ptr->index;
        from_item_set->next[next_symbol] = existing_item_set_ptr;
    } else {
        next_item_set_ptr->index  = m_lr0_itemsets.size();
        go_to_table[from_item_set->index][next_symbol] = next_item_set_ptr->index;
        from_item_set->next[next_symbol] = next_item_set_ptr.get();
        m_lr0_itemsets[next_item_set_ptr->kernel] = std::move(next_item_set_ptr);
        return from_item_set->next[next_symbol];
    }
    return nullptr;
}

void LALR1Parser::generate_first_sets() {
    for (uint32_t const& s : m_terminals) {
        m_firsts.insert(std::pair<uint32_t, std::set<uint32_t>>(s, {s}));
    }
    bool changed = true;
    while (changed) {
        changed = false;
        for (const std::unique_ptr<Production>& p : m_productions) {
            std::set<uint32_t>& f = m_firsts[p->head];
            if (p->is_epsilon()) {
                changed = changed || m_nullable.insert(p->head).second;
                continue;
            }
            size_t old = f.size();
            size_t i = 0;
            for (uint32_t const& s : p->body) {
                std::set<uint32_t>& f2 = m_firsts[s];
                f.insert(f2.begin(), f2.end());
                if (m_nullable.find(s) == m_nullable.end()) {
                    break;
                }
                i++;
            }
            if (i == p->body.size()) {
                changed = changed || m_nullable.insert(p->head).second;
            }
            changed = changed || (f.size() != old);
        }
    }
}

void LALR1Parser::generate_lr1_itemsets() {
    for (std::map<std::set<Item>,std::unique_ptr<ItemSet>>::value_type const& kv : m_lr0_itemsets) {
        for (Item const& l0_item : kv.second->kernel) {
            ItemSet temp_item_set;
            temp_item_set.kernel.insert(l0_item);
            generate_lr1_closure(&temp_item_set);
            for (Item const& l1_item : temp_item_set.closure) {
                if (l1_item.lookahead != LALR1Parser::NULL_SYMBOL) {
                    m_spontaneous_map[l1_item.production].insert(l1_item.lookahead);
                } else {
                    if (l1_item.dot < l1_item.production->body.size()) {
                        Item temp_item(l1_item.production, l1_item.dot + 1, LALR1Parser::NULL_SYMBOL);
                        m_propagate_map[l0_item].insert(temp_item);
                    }
                }
            }
        }
    }
    std::map<Item, std::set<int>> lookaheads;
    for (std::map<std::set<Item>,std::unique_ptr<ItemSet>>::value_type const& kv : m_lr0_itemsets) {
        for (Item const& l0_item : kv.second->kernel) {
            lookaheads[l0_item].insert(m_spontaneous_map[l0_item.production].begin(),
                                       m_spontaneous_map[l0_item.production].end());
            if (l0_item.production == m_productions[root_production_id].get()) {
                lookaheads[l0_item].insert(Lexer::TOKEN_END_ID);
            }
        }
    }
    bool changed = true;
    while (changed) {
        changed = false;
        for (std::map<Item, std::set<Item>>::value_type& kv : m_propagate_map) {
            Item item_from = kv.first;
            for (Item const& item_to : kv.second) {
                size_t size_before = lookaheads[item_to].size();
                lookaheads[item_to].insert(lookaheads[item_from].begin(), lookaheads[item_from].end());
                size_t size_after = lookaheads[item_to].size();
                changed = changed || size_after > size_before;
            }
        }
    }
    for (std::map<std::set<Item>,std::unique_ptr<ItemSet>>::value_type const& kv : m_lr0_itemsets) {
        std::unique_ptr<ItemSet> lr1_item_set_ptr = std::make_unique<ItemSet>();
        for (Item const& l0_item : kv.second->kernel) {
            for (int const& lookahead : lookaheads[l0_item]) {
                Item lr1_item(l0_item.production, l0_item.dot, lookahead);
                lr1_item_set_ptr->kernel.insert(lr1_item);
            }
            if (l0_item.production == m_productions[root_production_id].get() && l0_item.dot == 0) {
                root_itemset_ptr = lr1_item_set_ptr.get();
            }
        }
        generate_lr1_closure(lr1_item_set_ptr.get());
        lr1_item_set_ptr->index = kv.second->index;
        m_lr1_itemsets[lr1_item_set_ptr->kernel] = std::move(lr1_item_set_ptr);
    }
    // this seems like the wrong way to do this still:
    for (std::map<std::set<Item>,std::unique_ptr<ItemSet>>::value_type const& kv1 : m_lr1_itemsets) {
        for (std::map<int, int>::value_type next_index: go_to_table[kv1.second->index]) {
            bool success = false;
            for (std::map<std::set<Item>,std::unique_ptr<ItemSet>>::value_type const& kv2 : m_lr1_itemsets) {
                if (next_index.second == kv2.second->index) {
                    kv1.second->next[next_index.first] = kv2.second.get();
                    success = true;
                    break;
                }
            }
            assert(success);
        }
    }
}

void LALR1Parser::generate_lr1_closure(ItemSet* item_set_ptr) {
    std::deque<Item> queue(item_set_ptr->kernel.begin(), item_set_ptr->kernel.end());
    while (!queue.empty()) {
        Item item = queue.back();
        queue.pop_back();
        uint32_t next_symbol;
        if (lr_closure_helper(item_set_ptr, &item, &next_symbol)) {
            continue;
        }
        std::vector<uint32_t> lookaheads;
        size_t pos = item.dot + 1;
        while (pos < item.production->body.size()) {
            uint32_t symbol = item.production->body.at(pos);
            std::set<uint32_t> symbol_firsts = m_firsts.find(symbol)->second;
            lookaheads.insert(lookaheads.end(), std::make_move_iterator(symbol_firsts.begin()),
                                                std::make_move_iterator(symbol_firsts.end()));
            if (m_nullable.find(symbol) == m_nullable.end()) {
                break;
            }
            pos++;
        }
        if (pos == item.production->body.size()) {
            lookaheads.push_back(item.lookahead);
        }
        for (Production* const p : m_nonterminals.at(next_symbol)) {
            for (uint32_t const &l: lookaheads) {
                queue.emplace_back(p, 0, l);
            }
        }
    }
}

void LALR1Parser::generate_lalr1_parsing_table() {
    generate_lalr1_goto();
    generate_lalr1_action();
}

void LALR1Parser::generate_lalr1_goto() {
    // done already at end of generate_lr1_itemsets()?
}

// Dragon book page 253
void LALR1Parser::generate_lalr1_action() {
    for (std::map<std::set<Item>,std::unique_ptr<ItemSet>>::value_type const& kv : m_lr1_itemsets) {
        ItemSet* item_set_ptr = kv.second.get();
        item_set_ptr->actions.resize(m_lexer.symbol_id.size(), false);
        for (Item const& item : item_set_ptr->closure) {
            if (!item.has_dot_at_end()) {
                if (m_terminals.find(item.next_symbol()) == m_terminals.end() &&
                   m_nonterminals.find(item.next_symbol()) == m_nonterminals.end()) {
                    continue;
                }
                assert(item_set_ptr->next.find(item.next_symbol()) != item_set_ptr->next.end());
                Action& action = item_set_ptr->actions[item.next_symbol()];
                if (!std::holds_alternative<bool>(action)) {
                    if (std::holds_alternative<ItemSet*>(action) && std::get<ItemSet*>(action) == item_set_ptr->next[item.next_symbol()]) {
                        continue;
                    }
                    std::cout << "Warning: For symbol " << m_lexer.id_symbol[item.next_symbol()] << ", adding shift to "
                              << item_set_ptr->next[item.next_symbol()]->index << " causes ";
                    if (std::holds_alternative<ItemSet*>(action)) {
                        std::cout << "shift-shift conflict with shift to " << std::get<ItemSet*>(action)->index << std::endl;
                    } else {
                        std::cout << "shift-reduce conflict with reduction " << m_lexer.id_symbol[std::get<Production*>(action)->head]
                                  << "-> {";
                        for (uint32_t symbol : std::get<Production*>(action)->body) {
                            std::cout << m_lexer.id_symbol[symbol] << ",";
                        }
                        std::cout << "}" << std::endl;
                    }
                }
                item_set_ptr->actions[item.next_symbol()] = item_set_ptr->next[item.next_symbol()];
            }
            if (item.has_dot_at_end()) {
                if (item.production == m_productions[root_production_id].get()) {
                    Action action = true;
                    item_set_ptr->actions[Lexer::TOKEN_END_ID] = action;
                } else {
                    Action& action = item_set_ptr->actions[item.lookahead];
                    if (!std::holds_alternative<bool>(action)) {
                        std::cout << "Warning: For symbol " << m_lexer.id_symbol[item.lookahead]
                                  << ", adding reduction " << m_lexer.id_symbol[item.production->head] << "-> {";
                        for (uint32_t symbol : item.production->body) {
                            std::cout << m_lexer.id_symbol[symbol] << ",";
                        }
                        std::cout << "} causes ";
                        if (std::holds_alternative<ItemSet*>(action)) {
                            std::cout << "shift-reduce conflict with shift to " << std::get<ItemSet*>(action)->index << std::endl;
                        } else {
                            std::cout << "reduce-reduce conflict with reduction "
                                      << m_lexer.id_symbol[std::get<Production*>(action)->head]
                                      << "-> {";
                            for (uint32_t symbol : std::get<Production*>(action)->body) {
                                std::cout << m_lexer.id_symbol[symbol] << ",";
                            }
                            std::cout << "}" << std::endl;
                        }
                    }
                    item_set_ptr->actions[item.lookahead] = item.production;
                }
            }
        }
    }
}

static uint32_t get_line_num(MatchedSymbol& top_symbol) {
    uint32_t line_num = -1;
    std::stack<MatchedSymbol> symbols;
    symbols.push(std::move(top_symbol));
    while (line_num == -1) {
        assert(!symbols.empty());
        MatchedSymbol& curr_symbol = symbols.top();
        std::visit(overloaded {
            [&line_num](Token& token) {
                line_num = token.line;
            },
            [&symbols](NonTerminal& m) {
                for (int i = 0; i < m.production->body.size(); i++) {
                    symbols.push(std::move(NonTerminal::all_children[m.children_start + i]));
                }
            }
        }, curr_symbol);
        symbols.pop();
    }
    return line_num;
}

std::string LALR1Parser::get_input_after_last_newline(std::stack<MatchedSymbol>& parse_stack_matches) {
    std::string error_message_reversed;
    bool done = false;
    while (!parse_stack_matches.empty() && !done) {
        MatchedSymbol top_symbol = std::move(parse_stack_matches.top());
        parse_stack_matches.pop();
        std::visit(overloaded {
                [&error_message_reversed, &done](Token& token) {
                    if (token.get_string() =="\r" || token.get_string() == "\n") {
                        done = true;
                    } else {
                        // input is being read backwards, so reverse each token so that when the entire input is reversed
                          // each token is displayed correctly
                        std::string token_string = token.get_string();
                        std::reverse(token_string.begin(), token_string.end());
                        error_message_reversed += token_string;
                    }
                },
                [&parse_stack_matches](NonTerminal& m) {
                    for (int i = 0; i < m.production->body.size(); i++) {
                        parse_stack_matches.push(std::move(NonTerminal::all_children[m.children_start + i]));
                    }
                }
        }, top_symbol);
    }
    std::reverse(error_message_reversed.begin(), error_message_reversed.end());
    return error_message_reversed;
}

std::string LALR1Parser::get_input_until_next_newline(ReaderInterface* reader, Token* error_token) {
    std::string rest_of_line;
    bool next_is_end_token = (error_token->type_ids->at(0) == Lexer::TOKEN_END_ID);
    bool next_has_newline = (error_token->get_string().find('\n') != std::string::npos) || (error_token->get_string().find('\r') != std::string::npos);
    while (!next_has_newline && !next_is_end_token) {
        Token token = get_next_symbol();
        next_has_newline = (token.get_string().find('\n') != std::string::npos) || (token.get_string().find('\r') != std::string::npos);
        if (!next_has_newline) {
            rest_of_line += token.get_string();
            next_is_end_token = (token.type_ids->at(0) == Lexer::TOKEN_END_ID);
        }
    }
    rest_of_line += "\n";
    return rest_of_line;
}

std::string unescape(char const& c) {
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
            return {1, c};
    }
}

std::string LALR1Parser::report_error(ReaderInterface* reader) {
    assert(m_next_token == std::nullopt);
    assert(!m_parse_stack_matches.empty());
    MatchedSymbol top_symbol = std::move(m_parse_stack_matches.top());
    m_parse_stack_matches.pop();
    uint32_t line_num = get_line_num(top_symbol);
    Token token = std::get<Token>(top_symbol);
    std::string consumed_input = get_input_after_last_newline(m_parse_stack_matches);
    std::string error_type = "unknown error";
    std::string error_indicator;
    Token error_token = token;
    std::string rest_of_line = get_input_until_next_newline(reader, &error_token);
    for (uint32_t i = 0; i < consumed_input.size() + 10; i++) {
        error_indicator += " ";
    }
    error_indicator +=  "^\n";
    if (token.type_ids->at(0) == Lexer::TOKEN_END_ID && consumed_input.empty()) {
        error_type = "empty file";
        error_indicator = "\n^\n";
    } else {
        error_type = "expected ";
        for (uint32_t i = 0; i < m_parse_stack_states.top()->actions.size(); i++) {
            Action action = m_parse_stack_states.top()->actions[i];
            if (action.index() != 0) {
                /*
                if (it.first == m_lexer.symbol_id["Space"]) {
                    continue;
                }
                */
                error_type += "'";
                if (auto* regex_ast_literal = dynamic_cast<RegexASTLiteral*>(m_lexer.get_rule(i))) {
                    error_type += unescape(char(regex_ast_literal->character));
                } else {
                    error_type += m_lexer.id_symbol[i];
                }
                error_type += "',";
            }
        }
        error_type.pop_back();
        error_type += " before '" + unescape(token.get_string()[0]) + "' token";
    }
    std::string file_name = boost::filesystem::canonical((dynamic_cast<FileReader*>(reader))->get_path()).string();
    std::string error_string = file_name + ":" + std::to_string(line_num + 1) + ":"
                               + std::to_string(consumed_input.size() + 1) + ": error: " + error_type + "\n";
    for (int i = 0; i < 10; i++) {
        error_string += " ";
    }
    error_string += consumed_input + error_token.get_string() + rest_of_line + error_indicator;
    return error_string;
}

NonTerminal LALR1Parser::parse(ReaderInterface* reader) {
    reset(reader);
    m_parse_stack_states.push(root_itemset_ptr);
    bool accept = false;
    while (true) {
        Token next_terminal = std::move(get_next_symbol());
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

void LALR1Parser::reset(ReaderInterface* reader) {
    m_next_token = std::nullopt;
    while (!m_parse_stack_states.empty()) {
        m_parse_stack_states.pop();
    }
    while (!m_parse_stack_matches.empty()) {
        m_parse_stack_matches.pop();
    }
    m_lexer.reset(reader); 
}

Token LALR1Parser::get_next_symbol() {
    if (m_next_token == std::nullopt) {
        Token token = m_lexer.scan();
        return token;
    }
    Token s = std::move(m_next_token.value());
    m_next_token = std::nullopt;
    return s;
}

bool LALR1Parser::parse_advance(Token& next_token, bool* accept) {
    for (int const& type : *(next_token.type_ids)) {
        if (parse_symbol(type, next_token, accept)) {
            return (*accept);
        }
    }
    assert(*accept == false);
    // For error handling
    m_parse_stack_matches.push(std::move(next_token));
    return true;
}

bool LALR1Parser::parse_symbol(uint32_t const& type_id, Token& next_token, bool* accept) {
    ItemSet* curr = m_parse_stack_states.top();
    Action& it = curr->actions[type_id];
    bool ret;
    std::visit(overloaded {
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
            size_t n = reduce->body.size();
            for (size_t i = 0; i < n; i++) {
                m_parse_stack_states.pop();
                NonTerminal::all_children[matched_nonterminal.children_start + n - i - 1] = std::move(m_parse_stack_matches.top());
                m_parse_stack_matches.pop();// matched_nonterminal.children->at(n - i - 1) = std::move(m);
            }
            if (reduce->semantic_rule != nullptr) {
                m_lexer.m_reduce_pos = m_next_token->start_pos - 1;
                matched_nonterminal.value = reduce->semantic_rule(&matched_nonterminal);
            }
            {
                ItemSet* curr = m_parse_stack_states.top();
                Action const& it = curr->actions[matched_nonterminal.production->head];
                m_parse_stack_states.push(std::get<ItemSet*>(it));
                m_parse_stack_matches.push(std::move(matched_nonterminal));
            }
            ret = true;
            return;
        }
    }, it);
    return ret;
}
