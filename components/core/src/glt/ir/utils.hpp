#ifndef CLP_IR_UTILS_HPP
#define CLP_IR_UTILS_HPP

#include <string_view>

namespace clp::ir {
/**
 * @param buf
 * @return Whether the content in the buffer starts with one of the IR stream magic numbers
 */
auto has_ir_stream_magic_number(std::string_view buf) -> bool;
}  // namespace clp::ir

#endif  // CLP_IR_UTILS_HPP
