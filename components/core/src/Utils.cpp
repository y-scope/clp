#include "Utils.hpp"

// C libraries
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

// C++ libraries
#include <algorithm>
#include <iostream>
#include <set>

// Boost libraries
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

// spdlog
#include <spdlog/spdlog.h>

// Log surgeon
#include <log_surgeon/SchemaParser.hpp>

// Project headers
#include "spdlog_with_specializations.hpp"
#include "string_utils.hpp"

using std::list;
using std::string;
using std::vector;

ErrorCode create_directory (const string& path, mode_t mode, bool exist_ok) {
    int retval = mkdir(path.c_str(), mode);
    if (0 != retval) {
        if (EEXIST != errno) {
            return ErrorCode_errno;
        } else if (false == exist_ok) {
            return ErrorCode_FileExists;
        }
    }

    return ErrorCode_Success;
}

ErrorCode create_directory_structure (const string& path, mode_t mode) {
    assert(!path.empty());

    // Check if entire path already exists
    struct stat s = {};
    if (0 == stat(path.c_str(), &s)) {
        // Deepest directory exists, so can return here
        return ErrorCode_Success;
    } else if (ENOENT != errno) {
        // Unexpected error
        return ErrorCode_errno;
    }

    // Find deepest directory which exists, starting from the (2nd) deepest directory
    size_t path_end_pos = path.find_last_of('/');
    size_t last_path_end_pos = path.length();
    string dir_path;
    while (string::npos != path_end_pos) {
        if (last_path_end_pos - path_end_pos > 1) {
            dir_path.assign(path, 0, path_end_pos);
            if (0 == stat(dir_path.c_str(), &s)) {
                break;
            } else if (ENOENT != errno) {
                // Unexpected error
                return ErrorCode_errno;
            }
        }

        last_path_end_pos = path_end_pos;
        path_end_pos = path.find_last_of('/', path_end_pos - 1);
    }

    if (string::npos == path_end_pos) {
        // NOTE: Since the first path we create below contains more than one character, this assumes the path "/"
        // already exists
        path_end_pos = 0;
    }
    while (string::npos != path_end_pos) {
        path_end_pos = path.find_first_of('/', path_end_pos + 1);
        dir_path.assign(path, 0, path_end_pos);
        // Technically the directory shouldn't exist at this point in the code, but it may have been created concurrently.
        auto error_code = create_directory(dir_path, mode, true);
        if (ErrorCode_Success != error_code) {
            return error_code;
        }
    }

    return ErrorCode_Success;
}

bool get_bounds_of_next_var (const string& msg, size_t& begin_pos, size_t& end_pos) {
    const auto msg_length = msg.length();
    if (end_pos >= msg_length) {
        return false;
    }

    while (true) {
        begin_pos = end_pos;
        // Find next non-delimiter
        for (; begin_pos < msg_length; ++begin_pos) {
            if (false == is_delim(msg[begin_pos])) {
                break;
            }
        }
        if (msg_length == begin_pos) {
            // Early exit for performance
            return false;
        }

        bool contains_decimal_digit = false;
        bool contains_alphabet = false;

        // Find next delimiter
        end_pos = begin_pos;
        for (; end_pos < msg_length; ++end_pos) {
            char c = msg[end_pos];
            if (is_decimal_digit(c)) {
                contains_decimal_digit = true;
            } else if (is_alphabet(c)) {
                contains_alphabet = true;
            } else if (is_delim(c)) {
                break;
            }
        }

        // Treat token as variable if:
        // - it contains a decimal digit, or
        // - it's directly preceded by an equals sign and contains an alphabet, or
        // - it could be a multi-digit hex value
        if (contains_decimal_digit ||
            (begin_pos > 0 && '=' == msg[begin_pos - 1] && contains_alphabet) ||
            could_be_multi_digit_hex_value(msg, begin_pos, end_pos)) {
            break;
        }
    }

    return (msg_length != begin_pos);
}

string get_parent_directory_path (const string& path) {
    string dirname = get_unambiguous_path(path);

    size_t last_slash_pos = dirname.find_last_of('/');
    if (0 == last_slash_pos) {
        dirname = "/";
    } else if (string::npos == last_slash_pos) {
        dirname = ".";
    } else {
        dirname.resize(last_slash_pos);
    }

    return dirname;
}

string get_unambiguous_path (const string& path) {
    string unambiguous_path;
    if (path.empty()) {
        return unambiguous_path;
    }

    // Break path into components
    vector<string> path_components;
    boost::split(path_components, path, boost::is_any_of("/"), boost::token_compress_on);

    // Remove ambiguous components
    list<string> unambiguous_components;
    size_t num_components_to_ignore = 0;
    for (size_t i = path_components.size(); i-- > 0;) {
        if (".." == path_components[i]) {
            ++num_components_to_ignore;
        } else if ("." == path_components[i] || path_components[i].empty()) {
            // Do nothing
        } else if (num_components_to_ignore > 0) {
            --num_components_to_ignore;
        } else {
            unambiguous_components.emplace_front(path_components[i]);
        }
    }

    // Assemble unambiguous path from leading slash (if any) and the unambiguous components
    if ('/' == path[0]) {
        unambiguous_path += '/';
    }
    if (!unambiguous_components.empty()) {
        unambiguous_path += boost::join(unambiguous_components, "/");
    }

    return unambiguous_path;
}

ErrorCode read_list_of_paths (const string& list_path, vector<string>& paths) {
    FileReader file_reader;
    ErrorCode error_code = file_reader.try_open(list_path);
    if (ErrorCode_Success != error_code) {
        return error_code;
    }

    // Read file
    string line;
    while (true) {
        error_code = file_reader.try_read_to_delimiter('\n', false, false, line);
        if (ErrorCode_Success != error_code) {
            break;
        }
        // Only add non-empty paths
        if (line.empty() == false) {
            paths.push_back(line);
        }
    }
    // Check for any unexpected errors
    if (ErrorCode_EndOfFile != error_code) {
        return error_code;
    }

    file_reader.close();

    return ErrorCode_Success;
}

// TODO: duplicates code in log_surgeon/parser.tpp, should implement a
// SearchParser in log_surgeon instead and use it here. Specifically,
// initialization of lexer.m_symbol_id , contains_delimiter error, and add_rule
// logic.
void load_lexer_from_file (std::string schema_file_path,
                           bool reverse,
                           log_surgeon::lexers::ByteLexer& lexer) {
    FileReader schema_reader;
    schema_reader.try_open(schema_file_path);
    /// TODO: this wrapper is repeated a lot
    log_surgeon::Reader reader_wrapper{
        [&] (char* buf, size_t count, size_t& read_to) -> log_surgeon::ErrorCode {
            schema_reader.read(buf, count, read_to);
            if (read_to == 0) {
                return log_surgeon::ErrorCode::EndOfFile;
            }
            return log_surgeon::ErrorCode::Success;
        }
    };
    log_surgeon::SchemaParser sp;
    std::unique_ptr<log_surgeon::SchemaAST> schema_ast = sp.generate_schema_ast(reader_wrapper);
    auto* delimiters_ptr = dynamic_cast<log_surgeon::DelimiterStringAST*>(
            schema_ast->m_delimiters.get());
    if (!lexer.m_symbol_id.empty()) {
        throw std::runtime_error("Error: symbol_ids initialized before setting enum symbol_ids");
    }

    // cTokenEnd and cTokenUncaughtString never need to be added as a rule to
    // the lexer as they are not parsed
    lexer.m_symbol_id[log_surgeon::cTokenEnd] = (int)log_surgeon::SymbolID::TokenEndID;
    lexer.m_symbol_id[log_surgeon::cTokenUncaughtString] =
            (int)log_surgeon::SymbolID::TokenUncaughtStringID;
    // cTokenInt, cTokenFloat, cTokenFirstTimestamp, and cTokenNewlineTimestamp
    // each have unknown rule(s) until specified by the user so can't be
    // explicitly added and are done by looping over schema_vars (user schema)
    lexer.m_symbol_id[log_surgeon::cTokenInt] = (int)log_surgeon::SymbolID::TokenIntId;
    lexer.m_symbol_id[log_surgeon::cTokenFloat] = (int)log_surgeon::SymbolID::TokenFloatId;
    lexer.m_symbol_id[log_surgeon::cTokenFirstTimestamp] =
            (int)log_surgeon::SymbolID::TokenFirstTimestampId;
    lexer.m_symbol_id[log_surgeon::cTokenNewlineTimestamp] =
            (int)log_surgeon::SymbolID::TokenNewlineTimestampId;
    // cTokenNewline is not added in schema_vars and can be explicitly added
    // as '\n' to catch the end of non-timestamped log messages
    lexer.m_symbol_id[log_surgeon::cTokenNewline] = (int)log_surgeon::SymbolID::TokenNewlineId;

    lexer.m_id_symbol[(int)log_surgeon::SymbolID::TokenEndID] = log_surgeon::cTokenEnd;
    lexer.m_id_symbol[(int)log_surgeon::SymbolID::TokenUncaughtStringID] =
            log_surgeon::cTokenUncaughtString;
    lexer.m_id_symbol[(int)log_surgeon::SymbolID::TokenIntId] = log_surgeon::cTokenInt;
    lexer.m_id_symbol[(int)log_surgeon::SymbolID::TokenFloatId] = log_surgeon::cTokenFloat;
    lexer.m_id_symbol[(int)log_surgeon::SymbolID::TokenFirstTimestampId] =
            log_surgeon::cTokenFirstTimestamp;
    lexer.m_id_symbol[(int)log_surgeon::SymbolID::TokenNewlineTimestampId] =
            log_surgeon::cTokenNewlineTimestamp;
    lexer.m_id_symbol[(int)log_surgeon::SymbolID::TokenNewlineId] = log_surgeon::cTokenNewline;

    lexer.add_rule(lexer.m_symbol_id["newLine"],
                   std::move(std::make_unique<log_surgeon::finite_automata::RegexASTLiteral<
                           log_surgeon::finite_automata::RegexNFAByteState>>(
                           log_surgeon::finite_automata::RegexASTLiteral<
                                   log_surgeon::finite_automata::RegexNFAByteState>('\n'))));

    if (delimiters_ptr != nullptr) {
        lexer.add_delimiters(delimiters_ptr->m_delimiters);
    }
    for (std::unique_ptr<log_surgeon::ParserAST> const& parser_ast : schema_ast->m_schema_vars) {
        auto* rule = dynamic_cast<log_surgeon::SchemaVarAST*>(parser_ast.get());

        if ("timestamp" == rule->m_name) {
            continue;
        }

        if (lexer.m_symbol_id.find(rule->m_name) == lexer.m_symbol_id.end()) {
            lexer.m_symbol_id[rule->m_name] = lexer.m_symbol_id.size();
            lexer.m_id_symbol[lexer.m_symbol_id[rule->m_name]] = rule->m_name;
        }

        // transform '.' from any-character into any non-delimiter character
        rule->m_regex_ptr->remove_delimiters_from_wildcard(delimiters_ptr->m_delimiters);

        bool is_possible_input[log_surgeon::cUnicodeMax] = {false};
        rule->m_regex_ptr->set_possible_inputs_to_true(is_possible_input);
        bool contains_delimiter = false;
        uint32_t delimiter_name;
        for (uint32_t delimiter : delimiters_ptr->m_delimiters) {
            if (is_possible_input[delimiter]) {
                contains_delimiter = true;
                delimiter_name = delimiter;
                break;
            }
        }

        if (contains_delimiter) {
            FileReader schema_reader;
            ErrorCode error_code = schema_reader.try_open(schema_ast->m_file_path);
            if (ErrorCode_Success != error_code) {
                throw std::runtime_error(
                        schema_file_path + ":" + std::to_string(rule->m_line_num + 1) +
                        ": error: '" + rule->m_name
                        + "' has regex pattern which contains delimiter '" + char(delimiter_name) +
                        "'.\n");
            } else {
                // more detailed debugging based on looking at the file
                string line;
                for (uint32_t i = 0; i <= rule->m_line_num; i++) {
                    schema_reader.read_to_delimiter('\n', false, false, line);
                }
                int colon_pos = 0;
                for (char i : line) {
                    colon_pos++;
                    if (i == ':') {
                        break;
                    }
                }
                string indent(10, ' ');
                string spaces(colon_pos, ' ');
                string arrows(line.size() - colon_pos, '^');

                throw std::runtime_error(
                        schema_file_path + ":" + std::to_string(rule->m_line_num + 1) +
                        ": error: '" + rule->m_name
                        + "' has regex pattern which contains delimiter '" + char(delimiter_name) +
                        "'.\n"
                        + indent + line + "\n" + indent + spaces + arrows + "\n");
            }
        }
        lexer.add_rule(lexer.m_symbol_id[rule->m_name], std::move(rule->m_regex_ptr));
    }
    if (reverse) {
        lexer.generate_reverse();
    } else {
        lexer.generate();
    }
    schema_reader.close();
}
