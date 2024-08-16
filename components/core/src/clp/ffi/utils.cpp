#include "utils.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>

#include <msgpack.hpp>

#include "../utf8_utils.hpp"

using std::string;
using std::string_view;

namespace clp::ffi {
namespace {
/**
 * Serializes and appends a msgpack object to the given JSON string.
 * NOTE: Event if the serialization failed, `json_str` may be modified.
 * @param obj
 * @param json_str Outputs the appended JSON string.
 * @return true on success.
 * @return false if the type of the object is not supported, or the serialization failed.
 */
[[nodiscard]] auto serialize_and_append_msgpack_object_to_json_str(
        msgpack::object const& obj,
        string& json_str
) -> bool;

/**
 * Wrapper of `validate_and_append_escaped_utf8_string`, with both leading and end double quote
 * marks added to match JSON string spec.
 * NOTE: Event if the serialization failed, `json_str` may be modified.
 * @param src
 * @param json_str Outputs the appended JSON string.
 * @return Same as `validate_and_append_escaped_utf8_string`.
 */
[[nodiscard]] auto
append_escaped_utf8_string_to_json_str(string_view src, string& json_str) -> bool;

// NOLINTNEXTLINE(misc-no-recursion)
auto serialize_and_append_msgpack_object_to_json_str(
        msgpack::object const& obj,
        std::string& json_str
) -> bool {
    bool ret_val{true};
    switch (obj.type) {
        case msgpack::type::MAP:
            ret_val = serialize_and_append_msgpack_map_to_json_str(obj, json_str);
            break;
        case msgpack::type::ARRAY:
            ret_val = serialize_and_append_msgpack_array_to_json_str(obj, json_str);
            break;
        case msgpack::type::NIL:
            json_str += "null";
            break;
        case msgpack::type::BOOLEAN:
            json_str += obj.as<bool>() ? "true" : "false";
            break;
        case msgpack::type::STR:
            ret_val = append_escaped_utf8_string_to_json_str(obj.as<std::string_view>(), json_str);
            break;
        case msgpack::type::FLOAT32:
        case msgpack::type::FLOAT:
            json_str += std::to_string(obj.as<double>());
            break;
        case msgpack::type::POSITIVE_INTEGER:
            json_str += std::to_string(obj.as<uint64_t>());
            break;
        case msgpack::type::NEGATIVE_INTEGER:
            json_str += std::to_string(obj.as<int64_t>());
            break;
        default:
            ret_val = false;
            break;
    }
    return ret_val;
}

auto append_escaped_utf8_string_to_json_str(string_view src, string& json_str) -> bool {
    json_str.push_back('"');
    if (false == validate_and_append_escaped_utf8_string(src, json_str)) {
        return false;
    }
    json_str.push_back('"');
    return true;
}
}  // namespace

auto validate_and_escape_utf8_string(string_view raw) -> std::optional<string> {
    std::optional<std::string> ret_val;
    auto& escaped{ret_val.emplace()};
    escaped.reserve(raw.size() + (raw.size() / 2));
    if (false == validate_and_append_escaped_utf8_string(raw, escaped)) {
        return std::nullopt;
    }
    return ret_val;
}

auto validate_and_append_escaped_utf8_string(std::string_view src, std::string& dst) -> bool {
    string_view::const_iterator next_char_to_copy_it{src.cbegin()};

    auto escape_handler = [&](string_view::const_iterator it) -> void {
        // Allocate 6 + 1 size buffer to format control characters as "\u00bb", with the last byte
        // used by `snprintf` to append '\0'
        constexpr size_t cControlCharacterBufSize{7};
        std::array<char, cControlCharacterBufSize> buf{};
        std::string_view escaped_char;
        bool escape_required{true};
        switch (*it) {
            case '\b':
                escaped_char = "\\b";
                break;
            case '\t':
                escaped_char = "\\t";
                break;
            case '\n':
                escaped_char = "\\n";
                break;
            case '\f':
                escaped_char = "\\f";
                break;
            case '\r':
                escaped_char = "\\r";
                break;
            case '\\':
                escaped_char = "\\\\";
                break;
            case '"':
                escaped_char = "\\\"";
                break;
            default: {
                constexpr uint8_t cLargestControlCharacter{0x1F};
                auto const byte{static_cast<uint8_t>(*it)};
                if (cLargestControlCharacter >= byte) {
                    std::ignore = snprintf(buf.data(), buf.size(), "\\u00%02x", byte);
                    escaped_char = {buf.data(), buf.size() - 1};
                } else {
                    escape_required = false;
                }
                break;
            }
        }
        if (escape_required) {
            dst.append(next_char_to_copy_it, it);
            dst += escaped_char;
            next_char_to_copy_it = it + 1;
        }
    };

    if (false == validate_utf8_string(src, escape_handler)) {
        return false;
    }

    if (src.cend() != next_char_to_copy_it) {
        dst.append(next_char_to_copy_it, src.cend());
    }

    return true;
}

// NOLINTNEXTLINE(misc-no-recursion)
auto serialize_and_append_msgpack_array_to_json_str(
        msgpack::object const& array,
        std::string& json_str
) -> bool {
    if (msgpack::type::ARRAY != array.type) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto const array_data{array.via.array};
    bool is_first_element{true};
    json_str.push_back('[');
    for (auto const& element : std::span{array_data.ptr, static_cast<size_t>(array_data.size)}) {
        if (is_first_element) {
            is_first_element = false;
        } else {
            json_str.push_back(',');
        }
        if (false == serialize_and_append_msgpack_object_to_json_str(element, json_str)) {
            return false;
        }
    }
    json_str.push_back(']');
    return true;
}

// NOLINTNEXTLINE(misc-no-recursion)
auto serialize_and_append_msgpack_map_to_json_str(msgpack::object const& map, std::string& json_str)
        -> bool {
    if (msgpack::type::MAP != map.type) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    auto const& map_data{map.via.map};
    bool is_first_element{true};
    json_str.push_back('{');
    for (auto const& [key, val] : std::span{map_data.ptr, static_cast<size_t>(map_data.size)}) {
        if (is_first_element) {
            is_first_element = false;
        } else {
            json_str.push_back(',');
        }
        if (false == append_escaped_utf8_string_to_json_str(key.as<std::string_view>(), json_str)) {
            return false;
        }
        json_str.push_back(':');
        if (false == serialize_and_append_msgpack_object_to_json_str(val, json_str)) {
            return false;
        }
    }
    json_str.push_back('}');
    return true;
}
}  // namespace clp::ffi
