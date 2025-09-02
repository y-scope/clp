#ifndef CLP_S_UTILS_HPP
#define CLP_S_UTILS_HPP

#include <array>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace clp_s {
class FileUtils {
public:
    /**
     * Finds all files in a directory
     * @param path
     * @param file_paths
     * @return true if successful, false otherwise
     */
    static bool
    find_all_files_in_directory(std::string const& path, std::vector<std::string>& file_paths);

    /**
     * Finds all archives in a directory, including the directory itself
     * @param path
     * @param archive_paths
     * @return true if successful, false otherwise
     */
    static bool find_all_archives_in_directory(
            std::string_view const path,
            std::vector<std::string>& archive_paths
    );

    /**
     * Gets the last non-empty component of a path, accounting for trailing forward slashes.
     *
     * For example:
     * ./foo/bar.baz -> bar.baz
     * ./foo/bar.baz/ -> bar.baz
     *
     * @param path
     * @param name Returned component name
     * @return true on success, false otherwise
     */
    static bool get_last_non_empty_path_component(std::string_view const path, std::string& name);
};

class UriUtils {
public:
    /**
     * Gets the last component of a uri.
     *
     * For example:
     * https://www.something.org/abc-xyz -> abc-xyz
     * https://www.something.org/aaa/bbb/abc-xyz?something=something -> abc-xyz
     *
     * @param uri
     * @param name Returned component name
     * @return true on success, false otherwise
     */
    static bool get_last_uri_component(std::string_view const uri, std::string& name);
};

class StringUtils {
public:
    /**
     * Checks if character is a hexadecimal (base-16) digit
     * @param c
     * @return true if c is a hexadecimal digit, false otherwise
     */
    static inline bool is_delim(char c) {
        return !(
                '+' == c || ('-' <= c && c <= '9') || ('A' <= c && c <= 'Z') || '\\' == c
                || '_' == c || ('a' <= c && c <= 'z')
        );
    }

    /**
     * Checks if the string could be a hexadecimal value
     * @param str
     * @param begin_pos
     * @param end_pos
     * @return true if str could be a hexadecimal value, false otherwise
     */
    static inline bool
    could_be_multi_digit_hex_value(std::string const& str, size_t begin_pos, size_t end_pos) {
        if (end_pos - begin_pos < 2) {
            return false;
        }

        for (size_t i = begin_pos; i < end_pos; ++i) {
            auto c = str[i];
            if (false
                == (('a' <= c && c <= 'f') || ('A' <= c && c <= 'F') || ('0' <= c && c <= '9')))
            {
                return false;
            }
        }

        return true;
    }

    /**
     * Returns bounds of next variable in given string
     * A variable is a token (word between two delimiters) that contains numbers or is directly
     * preceded by an equals sign
     * @param msg
     * @param begin_pos Begin position of last variable, changes to begin position of next variable
     * @param end_pos End position of last variable, changes to end position of next variable
     * @return true if a variable was found, false otherwise
     */
    static bool get_bounds_of_next_var(std::string const& msg, size_t& begin_pos, size_t& end_pos);

    /**
     * Escapes a string according to JSON string escaping rules and appends the escaped string to
     * a buffer. The input string can be either ascii or UTF-8.
     *
     * According to the JSON spec JSON strings must escape control sequences (characters 0x00
     * through 0x1f) as well as the '"' and '\' characters.
     *
     * This function escapes common control sequences like newline with short escape sequences
     * (e.g. \n) and less common control sequences with unicode escape sequences (e.g. \u001f). The
     * '"' and '\' characters are escaped with a backslash.
     *
     * @param destination
     * @param source
     */
    static void escape_json_string(std::string& destination, std::string_view const source);

private:
    /**
     * Converts a character into its two byte hexadecimal representation.
     * @param c
     * @return the two byte hexadecimal representation of c as an array of two characters.
     */
    static std::array<char, 2> char_to_hex(char c) {
        std::array<char, 2> ret;
        auto nibble_to_hex = [](char nibble) -> char {
            if ('\x00' <= nibble && nibble <= '\x09') {
                return '0' + (nibble - '\x00');
            } else {
                return 'a' + (nibble - '\x0a');
            }
        };

        return std::array<char, 2>{nibble_to_hex(0x0F & (c >> 4)), nibble_to_hex(0x0f & c)};
    }

    /**
     * Converts a character into a unicode escape sequence (e.g. \u0000) and appends the escape
     * sequences to the `destination` buffer.
     * @param destination
     * @param c
     */
    static void char_to_escaped_four_char_hex(std::string& destination, char c) {
        destination.append("\\u00");
        auto hex = char_to_hex(c);
        destination.append(hex.data(), hex.size());
    }
};

enum EvaluatedValue {
    True,
    False,
    Unknown
};

template <class T2, class T1>
inline T2 bit_cast(T1 t1) {
    static_assert(sizeof(T1) == sizeof(T2), "Must match size");
    static_assert(std::is_standard_layout<T1>::value, "Need to be standard layout");
    static_assert(std::is_standard_layout<T2>::value, "Need to be standard layout");

    T2 t2;
    std::memcpy(std::addressof(t2), std::addressof(t1), sizeof(T1));
    return t2;
}

/**
 * Writes a numeric value to a stringstream.
 * @param stream
 * @param value
 * @tparam ValueType
 */
template <typename ValueType>
void write_numeric_value(std::stringstream& stream, ValueType value) {
    stream.write(reinterpret_cast<char*>(&value), sizeof(value));
}

/**
 * A span of memory where the underlying memory may not be aligned correctly for type T.
 *
 * This class should be used whenever we need a view into some memory, and we do not know whether it
 * is aligned correctly for type T. If the alignment of the underlying memory is known std::span
 * should be used instead.
 *
 * In C++ creating a pointer to objects of type T that is not correctly aligned for type T is
 * undefined behaviour, as is dereferencing such a pointer. This class avoids this undefined
 * behaviour by using memcpy (which any modern compiler should be able to optimize away).
 *
 * For any modern x86 platform the performance difference between using std::span and
 * UnalignedMemSpan should be fairly minimal.
 *
 * @tparam T
 */
template <typename T>
class UnalignedMemSpan {
public:
    UnalignedMemSpan() = default;

    UnalignedMemSpan(char* begin, size_t size) : m_begin(begin), m_size(size) {}

    size_t size() const { return m_size; }

    T operator[](size_t i) const {
        T tmp;
        memcpy(&tmp, m_begin + i * sizeof(T), sizeof(T));
        return tmp;
    }

    UnalignedMemSpan<T> sub_span(size_t start, size_t size) const {
        return {m_begin + start * sizeof(T), size};
    }

private:
    char* m_begin{nullptr};
    size_t m_size{0};
};
}  // namespace clp_s
#endif  // CLP_S_UTILS_HPP
