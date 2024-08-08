#include "encoding_methods.hpp"

#include <cstdint>
#include <string_view>

#include "../ir/types.hpp"

using clp::ir::eight_byte_encoded_variable_t;
using clp::ir::four_byte_encoded_variable_t;
using std::string_view;

namespace clp::ffi {
auto encode_four_byte_float_as_eight_byte(four_byte_encoded_variable_t four_byte_encoded_var
) -> eight_byte_encoded_variable_t {
    uint8_t decimal_point_pos{};
    uint8_t num_digits{};
    uint32_t digits{};
    bool is_negative{};
    decode_float_properties(
            four_byte_encoded_var,
            is_negative,
            digits,
            num_digits,
            decimal_point_pos
    );

    return encode_float_properties<eight_byte_encoded_variable_t>(
            is_negative,
            digits,
            num_digits,
            decimal_point_pos
    );
}

auto encode_four_byte_integer_as_eight_byte(four_byte_encoded_variable_t four_byte_encoded_var
) -> eight_byte_encoded_variable_t {
    return static_cast<eight_byte_encoded_variable_t>(four_byte_encoded_var);
}
}  // namespace clp::ffi
