#include "utf8_utils.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace clp {
auto is_utf8_encoded(std::string_view str) -> bool {
    auto escape_handler = []([[maybe_unused]] std::string_view::const_iterator it) -> void {};
    return validate_utf8_string(str, escape_handler);
}

namespace utf8_utils_internal {
auto parse_and_validate_lead_byte(
        uint8_t byte,
        size_t& num_continuation_bytes,
        uint32_t& code_point,
        uint32_t& code_point_lower_bound,
        uint32_t& code_point_upper_bound
) -> bool {
    if ((byte & cFourByteUtf8CharHeaderMask) == cFourByteUtf8CharHeader) {
        num_continuation_bytes = 3;
        code_point = (~cFourByteUtf8CharHeaderMask & byte);
        code_point_lower_bound = cFourByteUtf8CharCodePointLowerBound;
        code_point_upper_bound = cFourByteUtf8CharCodePointUpperBound;
    } else if ((byte & cThreeByteUtf8CharHeaderMask) == cThreeByteUtf8CharHeader) {
        num_continuation_bytes = 2;
        code_point = (~cThreeByteUtf8CharHeaderMask & byte);
        code_point_lower_bound = cThreeByteUtf8CharCodePointLowerBound;
        code_point_upper_bound = cThreeByteUtf8CharCodePointUpperBound;
    } else if ((byte & cTwoByteUtf8CharHeaderMask) == cTwoByteUtf8CharHeader) {
        num_continuation_bytes = 1;
        code_point = (~cTwoByteUtf8CharHeaderMask & byte);
        code_point_lower_bound = cTwoByteUtf8CharCodePointLowerBound;
        code_point_upper_bound = cTwoByteUtf8CharCodePointUpperBound;
    } else {
        return false;
    }
    return true;
}

auto is_ascii_char(uint8_t byte) -> bool {
    return cOneByteUtf8CharCodePointUpperBound >= byte;
}

auto is_valid_utf8_continuation_byte(uint8_t byte) -> bool {
    return (byte & cUtf8ContinuationByteMask) == cUtf8ContinuationByteHeader;
}

auto parse_continuation_byte(uint32_t code_point, uint8_t continuation_byte) -> uint32_t {
    return (code_point << cUtf8NumContinuationByteCodePointBits)
           + (continuation_byte & cUtf8ContinuationByteCodePointMask);
}
}  // namespace utf8_utils_internal
}  // namespace clp
