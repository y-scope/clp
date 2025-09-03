#include "FloatFormatEncoding.hpp"

#include <cstdint>
#include <string_view>
#include <system_error>

#include <ystdlib/error_handling/Result.hpp>

namespace clp_s::float_format_encoding {
auto get_float_encoding(std::string_view float_str) -> ystdlib::error_handling::Result<uint16_t> {
    auto const dot_pos{float_str.find('.')};
    uint16_t format{0};

    // Check whether it is scientific; if so, whether the exponent is E or e
    size_t exp_pos{float_str.find_first_of("Ee")};
    if (std::string_view::npos != exp_pos) {
        // Exponent must be followed by an integer (e.g., "1E" or "1e+" are illegal)
        if (false
            == ((exp_pos + 1 < float_str.length() && std::isdigit(float_str[exp_pos + 1]))
                || (exp_pos + 2 < float_str.length()
                    && ('+' == float_str[exp_pos + 1] || '-' == float_str[exp_pos + 1])
                    && std::isdigit(static_cast<unsigned char>(float_str[exp_pos + 2])))))
        {
            return std::errc::protocol_not_supported;
        }

        format |= static_cast<uint16_t>(1u) << cExponentNotationPos;
        format |= static_cast<uint16_t>('E' == float_str[exp_pos] ? 1u : 0u)
                  << (cExponentNotationPos + 1);

        // Check whether there is a sign for the exponent
        if ('+' == float_str[exp_pos + 1]) {
            format |= static_cast<uint16_t>(1u) << cExponentSignPos;
        } else if ('-' == float_str[exp_pos + 1]) {
            format |= static_cast<uint16_t>(1u) << (cExponentSignPos + 1);
        }

        // Set the number of exponent digits
        int exp_digits = float_str.length() - exp_pos - 1;
        if (false == std::isdigit(static_cast<unsigned char>(float_str[exp_pos + 1]))) {
            exp_digits--;
        }
        format |= (static_cast<uint16_t>(std::min(exp_digits - 1, 3)) & static_cast<uint16_t>(0x03))
                  << cNumExponentDigitsPos;
    } else {
        exp_pos = float_str.length();
    }

    // Find first non-zero digit position
    size_t first_non_zero_frac_digit_pos{0ULL};
    if (false == std::isdigit(static_cast<unsigned char>(float_str[0]))) {
        first_non_zero_frac_digit_pos = 1;  // Skip sign
    }

    if ('0' == float_str[first_non_zero_frac_digit_pos]) {
        // We don't support prefix zeroes of the form 0N.Y
        if (first_non_zero_frac_digit_pos + 1 < float_str.length()
            && std::isdigit(
                    static_cast<unsigned char>(float_str[first_non_zero_frac_digit_pos + 1])
            ))
        {
            return std::errc::protocol_not_supported;
        }

        // For "0.xxx", find the first non-zero digit after the decimal
        if (std::string_view::npos != dot_pos) {
            for (size_t i{dot_pos + 1}; i < exp_pos; ++i) {
                if ('0' != float_str[i]) {
                    first_non_zero_frac_digit_pos = i;
                    break;
                }
            }
        }
    }

    int significant_digits = exp_pos - first_non_zero_frac_digit_pos;
    assert(first_non_zero_frac_digit_pos < exp_pos);
    if (std::string_view::npos != dot_pos && first_non_zero_frac_digit_pos < dot_pos) {
        significant_digits--;
    }

    // Number of significant digits must be greater than zero (e.g., E0 or . is illegal)
    assert(significant_digits > 0);
    uint16_t const compressed_significant_digits
            = static_cast<uint16_t>(std::min(significant_digits - 1, 15)) & 0x0F;

    format |= static_cast<uint16_t>(compressed_significant_digits) << cNumSignificantDigitsPos;
    return format;
}
}  // namespace clp_s::float_format_encoding
