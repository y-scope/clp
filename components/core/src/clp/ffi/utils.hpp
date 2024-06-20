#ifndef CLP_FFI_UTILS_HPP
#define CLP_FFI_UTILS_HPP

#include <optional>
#include <string>
#include <string_view>

namespace clp::ffi {
/**
 * Escapes a UTF8 encoded string.
 * @param raw The raw string to escape.
 * @return The escaped string on success.
 * @return std::nullopt if the string contains none-UTF8 encoded byte sequence.
 */
[[nodiscard]] auto escape_utf8_string(std::string_view raw) -> std::optional<std::string>;
}  // namespace clp::ffi

#endif  // CLP_UTILS_HPP
