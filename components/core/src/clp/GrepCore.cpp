#include "GrepCore.hpp"

#include <cstddef>
#include <set>
#include <string>
#include <string_view>

#include <log_surgeon/Constants.hpp>
#include <string_utils/string_utils.hpp>

#include "ir/parsing.hpp"
#include "LogSurgeonReader.hpp"
#include "QueryToken.hpp"
#include "StringReader.hpp"

using clp::ir::is_delim;
using clp::string_utils::is_alphabet;
using clp::string_utils::is_wildcard;
using std::string;

namespace clp {
namespace {
/**
 * Wraps the tokens returned from the log_surgeon lexer, and stores the variable ids of the tokens
 * in a search query in a set. This allows for optimized search performance.
 */
class SearchToken : public log_surgeon::Token {
public:
    std::set<int> m_type_ids_set;
};
}  // namespace

bool GrepCore::get_bounds_of_next_potential_var(
        string const& value,
        size_t& begin_pos,
        size_t& end_pos,
        bool& is_var
) {
    auto const value_length = value.length();
    if (end_pos >= value_length) {
        return false;
    }

    is_var = false;
    bool contains_wildcard = false;
    while (false == is_var && false == contains_wildcard && begin_pos < value_length) {
        // Start search at end of last token
        begin_pos = end_pos;

        // Find next wildcard or non-delimiter
        bool is_escaped = false;
        for (; begin_pos < value_length; ++begin_pos) {
            char c = value[begin_pos];

            if (is_escaped) {
                is_escaped = false;

                if (false == is_delim(c)) {
                    // Found escaped non-delimiter, so reverse the index to retain the escape
                    // character
                    --begin_pos;
                    break;
                }
            } else if ('\\' == c) {
                // Escape character
                is_escaped = true;
            } else {
                if (is_wildcard(c)) {
                    contains_wildcard = true;
                    break;
                }
                if (false == is_delim(c)) {
                    break;
                }
            }
        }

        bool contains_decimal_digit = false;
        bool contains_alphabet = false;

        // Find next delimiter
        is_escaped = false;
        end_pos = begin_pos;
        for (; end_pos < value_length; ++end_pos) {
            char c = value[end_pos];

            if (is_escaped) {
                is_escaped = false;

                if (is_delim(c)) {
                    // Found escaped delimiter, so reverse the index to retain the escape character
                    --end_pos;
                    break;
                }
            } else if ('\\' == c) {
                // Escape character
                is_escaped = true;
            } else {
                if (is_wildcard(c)) {
                    contains_wildcard = true;
                } else if (is_delim(c)) {
                    // Found delimiter that's not also a wildcard
                    break;
                }
            }

            if (string_utils::is_decimal_digit(c)) {
                contains_decimal_digit = true;
            } else if (is_alphabet(c)) {
                contains_alphabet = true;
            }
        }

        // Treat token as a definite variable if:
        // - it contains a decimal digit, or
        // - it could be a multi-digit hex value, or
        // - it's directly preceded by an equals sign and contains an alphabet without a wildcard
        //   between the equals sign and the first alphabet of the token
        auto variable = static_cast<std::string_view>(value).substr(begin_pos, end_pos - begin_pos);
        if (contains_decimal_digit || ir::could_be_multi_digit_hex_value(variable)) {
            is_var = true;
        } else if (begin_pos > 0 && '=' == value[begin_pos - 1] && contains_alphabet) {
            // Find first alphabet or wildcard in token
            is_escaped = false;
            bool found_wildcard_before_alphabet = false;
            for (auto i = begin_pos; i < end_pos; ++i) {
                auto c = value[i];

                if (is_escaped) {
                    is_escaped = false;

                    if (is_alphabet(c)) {
                        break;
                    }
                } else if ('\\' == c) {
                    // Escape character
                    is_escaped = true;
                } else if (is_wildcard(c)) {
                    found_wildcard_before_alphabet = true;
                    break;
                }
            }

            if (false == found_wildcard_before_alphabet) {
                is_var = true;
            }
        }
    }

    return (value_length != begin_pos);
}

bool GrepCore::get_bounds_of_next_potential_var(
        string const& value,
        size_t& begin_pos,
        size_t& end_pos,
        bool& is_var,
        log_surgeon::lexers::ByteLexer& lexer
) {
    size_t const value_length = value.length();
    if (end_pos >= value_length) {
        return false;
    }

    is_var = false;
    bool contains_wildcard = false;
    while (false == is_var && false == contains_wildcard && begin_pos < value_length) {
        // Start search at end of last token
        begin_pos = end_pos;

        // Find variable begin or wildcard
        bool is_escaped = false;
        for (; begin_pos < value_length; ++begin_pos) {
            char c = value[begin_pos];

            if (is_escaped) {
                is_escaped = false;

                if (false == lexer.is_delimiter(c)) {
                    // Found escaped non-delimiter, so reverse the index to retain the escape
                    // character
                    --begin_pos;
                    break;
                }
            } else if ('\\' == c) {
                // Escape character
                is_escaped = true;
            } else {
                if (is_wildcard(c)) {
                    contains_wildcard = true;
                    break;
                }
                if (false == lexer.is_delimiter(c)) {
                    break;
                }
            }
        }

        // Find next delimiter
        is_escaped = false;
        end_pos = begin_pos;
        for (; end_pos < value_length; ++end_pos) {
            char c = value[end_pos];

            if (is_escaped) {
                is_escaped = false;

                if (lexer.is_delimiter(c)) {
                    // Found escaped delimiter, so reverse the index to retain the escape character
                    --end_pos;
                    break;
                }
            } else if ('\\' == c) {
                // Escape character
                is_escaped = true;
            } else {
                if (is_wildcard(c)) {
                    contains_wildcard = true;
                } else if (lexer.is_delimiter(c)) {
                    // Found delimiter that's not also a wildcard
                    break;
                }
            }
        }

        if (end_pos > begin_pos) {
            bool has_prefix_wildcard = ('*' == value[begin_pos]) || ('?' == value[begin_pos]);
            bool has_suffix_wildcard = ('*' == value[end_pos - 1]) || ('?' == value[end_pos - 1]);
            bool has_wildcard_in_middle = false;
            for (size_t i = begin_pos + 1; i < end_pos - 1; ++i) {
                if (('*' == value[i] || '?' == value[i]) && value[i - 1] != '\\') {
                    has_wildcard_in_middle = true;
                    break;
                }
            }
            SearchToken search_token;
            if (has_wildcard_in_middle || has_prefix_wildcard) {
                // DO NOTHING
            } else {
                StringReader string_reader;
                LogSurgeonReader reader_wrapper(string_reader);
                log_surgeon::ParserInputBuffer parser_input_buffer;
                if (has_suffix_wildcard) {  // text*
                    // TODO: creating a string reader, setting it equal to a string, to read it into
                    // the ParserInputBuffer, seems like a convoluted way to set a string equal to a
                    // string, should be improved when adding a SearchParser to log_surgeon
                    string_reader.open(value.substr(begin_pos, end_pos - begin_pos - 1));
                    parser_input_buffer.read_if_safe(reader_wrapper);
                    lexer.reset();
                    lexer.scan_with_wildcard(parser_input_buffer, value[end_pos - 1], search_token);
                } else {  // no wildcards
                    string_reader.open(value.substr(begin_pos, end_pos - begin_pos));
                    parser_input_buffer.read_if_safe(reader_wrapper);
                    lexer.reset();
                    auto [err, token] = lexer.scan(parser_input_buffer);
                    if (log_surgeon::ErrorCode::Success != err) {
                        return false;
                    }
                    search_token = SearchToken{token.value()};
                    search_token.m_type_ids_set.insert(search_token.get_type_ids()->at(0));
                }
                auto const& type = search_token.get_type_ids()->at(0);
                if (type != static_cast<int>(log_surgeon::SymbolId::TokenUncaughtString)
                    && type != static_cast<int>(log_surgeon::SymbolId::TokenEnd))
                {
                    is_var = true;
                }
            }
        }
    }
    return (value_length != begin_pos);
}
}  // namespace clp
