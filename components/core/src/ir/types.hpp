#ifndef IR_TYPES_HPP
#define IR_TYPES_HPP

#include <cstdint>

namespace ir {
using epoch_time_ms_t = int64_t;
using eight_byte_encoded_variable_t = int64_t;
using four_byte_encoded_variable_t = int32_t;

enum class VariablePlaceholder : char {
    Integer = 0x11,
    Dictionary = 0x12,
    Float = 0x13,
    Escape = '\\',
};
}  // namespace ir

#endif  // IR_TYPES_HPP
