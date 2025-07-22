#include "SearchUtils.hpp"

#include <cmath>
#include <string>
#include <string_view>
#include <vector>

#include <simdjson.h>

#include "../../archive_constants.hpp"

namespace clp_s::search::ast {
namespace {
/**
 * Converts a four byte hex sequence to UTF-8.
 * @param hex
 * @param destination
 * @return true if the hex sequence could be converted to UTF-8 and false otherwise.
 */
[[nodiscard]] auto
convert_four_byte_hex_to_utf8(std::string_view const hex, std::string& destination) -> bool;

/**
 * Unescapes a KQL key or value with special handling for each case and append the unescaped
 * value to the `unescaped` buffer.
 * @param value
 * @param unescaped
 * @param is_value
 * @return true if the value was unescaped succesfully and false otherwise.
 */
[[nodiscard]] auto
unescape_kql_internal(std::string const& value, std::string& unescaped, bool is_value) -> bool;
}  // namespace

void splice_into(
        std::shared_ptr<Expression> const& parent,
        std::shared_ptr<Expression> const& child,
        OpList::iterator location
) {
    for (auto it = child->op_begin(); it != child->op_end(); it++) {
        auto sub_expr = std::static_pointer_cast<Expression>(*it);
        sub_expr->set_parent(parent.get());
    }
    parent->get_op_list().splice(location, child->get_op_list());
}

bool double_as_int(double in, FilterOperation op, int64_t& out) {
    switch (op) {
        case FilterOperation::EQ:
            out = static_cast<int64_t>(in);
            return in == static_cast<double>(out);
        case FilterOperation::LT:
        case FilterOperation::GTE:
            out = std::ceil(in);
        case FilterOperation::GT:
        case FilterOperation::LTE:
            out = std::floor(in);
        default:
            out = static_cast<int64_t>(in);
    }
    return true;
}

auto tokenize_column_descriptor(
        std::string const& descriptor,
        std::vector<std::string>& tokens,
        std::string& descriptor_namespace
) -> bool {
    std::string cur_tok;
    bool escaped{false};
    descriptor_namespace.clear();
    size_t start_index = 0;
    if (descriptor.size() > 0) {
        switch (descriptor.at(0)) {
            case constants::cAutogenNamespace.at(0):
            case constants::cRangeIndexNamespace.at(0):
            case constants::cReservedNamespace1.at(0):
            case constants::cReservedNamespace2.at(0):
                descriptor_namespace = descriptor.at(0);
                start_index += 1;
                break;
            default:
                break;
        }
    }

    for (size_t i = start_index; i < descriptor.size(); ++i) {
        if (false == escaped) {
            if ('\\' == descriptor[i]) {
                escaped = true;
            } else if ('.' == descriptor[i]) {
                if (cur_tok.empty()) {
                    return false;
                }
                std::string unescaped_token;
                if (unescape_kql_internal(cur_tok, unescaped_token, false)) {
                    tokens.push_back(unescaped_token);
                    cur_tok.clear();
                } else {
                    return false;
                }
            } else {
                cur_tok.push_back(descriptor[i]);
            }
            continue;
        }

        escaped = false;
        switch (descriptor[i]) {
            case '.':
                cur_tok.push_back('.');
                break;
            default:
                cur_tok.push_back('\\');
                cur_tok.push_back(descriptor[i]);
                break;
        }
    }

    if (escaped) {
        return false;
    }

    if (cur_tok.empty()) {
        return false;
    }

    std::string unescaped_token;
    if (unescape_kql_internal(cur_tok, unescaped_token, false)) {
        tokens.push_back(unescaped_token);
    } else {
        return false;
    }
    return true;
}

auto unescape_kql_value(std::string const& value, std::string& unescaped) -> bool {
    return unescape_kql_internal(value, unescaped, true);
}

auto has_unescaped_wildcards(std::string_view str) -> bool {
    for (size_t i = 0; i < str.size(); ++i) {
        if ('*' == str[i] || '?' == str[i]) {
            return true;
        }
        if ('\\' == str[i]) {
            ++i;
        }
    }
    return false;
}

namespace {
auto convert_four_byte_hex_to_utf8(std::string_view const hex, std::string& destination) -> bool {
    /**
     * We perform the conversion in this cumbersome way because c++20 deprecates most of the much
     * more convenient std::codecvt utilities.
     */
    std::string buf = "\"\\u";
    buf += hex;
    buf.push_back('"');
    buf.reserve(buf.size() + simdjson::SIMDJSON_PADDING);
    simdjson::ondemand::parser parser;
    auto value = parser.iterate(buf);
    try {
        if (false == value.is_scalar()) {
            return false;
        }

        if (simdjson::ondemand::json_type::string != value.type()) {
            return false;
        }

        std::string_view unescaped_utf8 = value.get_string(false);
        destination.append(unescaped_utf8);
    } catch (std::exception const& e) {
        return false;
    }
    return true;
}

auto unescape_kql_internal(std::string const& value, std::string& unescaped, bool is_value)
        -> bool {
    bool escaped{false};
    for (size_t i = 0; i < value.size(); ++i) {
        if (false == escaped) {
            if ('\\' == value[i]) {
                escaped = true;
            } else {
                unescaped.push_back(value[i]);
            }
            continue;
        }

        escaped = false;
        switch (value[i]) {
            case '\\':
                unescaped.append("\\\\");
                break;
            case '"':
                unescaped.push_back('"');
                break;
            case 't':
                unescaped.push_back('\t');
                break;
            case 'r':
                unescaped.push_back('\r');
                break;
            case 'n':
                unescaped.push_back('\n');
                break;
            case 'b':
                unescaped.push_back('\b');
                break;
            case 'f':
                unescaped.push_back('\f');
                break;
            case 'u': {
                size_t last_char_in_codepoint = i + 4;
                if (value.size() <= last_char_in_codepoint) {
                    return false;
                }

                auto four_byte_hex = std::string_view{value}.substr(i + 1, 4);
                i += 4;

                std::string tmp;
                if (false == convert_four_byte_hex_to_utf8(four_byte_hex, tmp)) {
                    return false;
                }

                // Make sure unicode escape sequences are always treated as literal characters
                if ("\\" == tmp) {
                    unescaped.append("\\\\");
                } else if ("?" == tmp && is_value) {
                    unescaped.append("\\?");
                } else if ("*" == tmp) {
                    unescaped.append("\\*");
                } else {
                    unescaped.append(tmp);
                }
                break;
            }
            case '{':
                unescaped.push_back('{');
                break;
            case '}':
                unescaped.push_back('}');
                break;
            case '(':
                unescaped.push_back('(');
                break;
            case ')':
                unescaped.push_back(')');
                break;
            case '<':
                unescaped.push_back('<');
                break;
            case '>':
                unescaped.push_back('>');
                break;
            case '*':
                unescaped.append("\\*");
                break;
            case '?':
                if (is_value) {
                    unescaped.append("\\?");
                } else {
                    unescaped.push_back('?');
                }
                break;
            case '@':
                unescaped.push_back('@');
                break;
            case '$':
                unescaped.push_back('$');
                break;
            case '!':
                unescaped.push_back('!');
                break;
            case '#':
                unescaped.push_back('#');
                break;
            default:
                return false;
        }
    }

    if (escaped) {
        return false;
    }
    return true;
}
}  // namespace
}  // namespace clp_s::search::ast
