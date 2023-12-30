#ifndef STRING_UTILS_TPP
#define STRING_UTILS_TPP

#include <charconv>

namespace string_utils {
template <typename integer_t>
bool convert_string_to_int(std::string_view raw, integer_t& converted) {
    auto raw_end = raw.cend();
    auto result = std::from_chars(raw.cbegin(), raw_end, converted);
    if (raw_end != result.ptr) {
        return false;
    } else {
        return result.ec == std::errc();
    }
}
}  // namespace string_utils

#endif  // STRING_UTILS_TPP
