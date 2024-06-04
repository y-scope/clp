#ifndef CLP_IR_UTILS_HPP
#define CLP_IR_UTILS_HPP

#include <string_view>

#include "LogEvent.hpp"

namespace clp::ir {
/**
 * @param buf
 * @return Whether the content in the buffer starts with one of the IR stream magic numbers
 */
auto has_ir_stream_magic_number(std::string_view buf) -> bool;

/**
 * @param buf
 * @return Extension name of IR
 */
auto get_ir_extension_name() -> std::string;

/**
 * Gets an approximated upper bound size of a given log message if it is encoded in IR format.
 * The approximation is based on the following assumptions:
 * 1. Dictionary variable lengths are all encoded using int32_t
 * 2. Timestamp or timestamp delta is encoded using int64_t
 * 3. Logtype length is encoded using int32_t
 * 4. The size of encoded variable roughly equals to their plain text size.
 * @param log_message
 * @param num_encoded_vars
 * @return the approximated ir size in bytes
 */

auto get_approximated_ir_size(std::string_view log_message, size_t num_encoded_vars) -> size_t;
}  // namespace clp::ir

#endif  // CLP_IR_UTILS_HPP
