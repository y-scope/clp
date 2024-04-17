#ifndef GLT_IR_UTILS_HPP
#define GLT_IR_UTILS_HPP

#include <string_view>

namespace glt::ir {
/**
 * @param buf
 * @return Whether the content in the buffer starts with one of the IR stream magic numbers
 */
auto has_ir_stream_magic_number(std::string_view buf) -> bool;
}  // namespace glt::ir

#endif  // GLT_IR_UTILS_HPP
