#ifndef IR_UTILS_HPP
#define IR_UTILS_HPP

#include <string_view>

namespace ir {
/**
 * @param buf
 * @return Whether the content in the buffer starts with one of the IR stream
 * magic numbers
 */
auto has_ir_stream_magic_number(std::string_view buf) -> bool;
}  // namespace ir

#endif  // IR_UTILS_HPP
