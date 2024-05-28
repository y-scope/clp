#ifndef CLP_CLP_IRDECOMPRESSION_HPP
#define CLP_CLP_IRDECOMPRESSION_HPP

#include <string>
#include <unordered_set>

#include "CommandLineArguments.hpp"

namespace clp::clp {
/**
 * Decompresses a file split into an IR into the given directory
 * @param command_line_args
 * @param file_orig_id
 * @return true if decompression was successful, false otherwise
 */
bool decompress_ir(CommandLineArguments& command_line_args, std::string const& file_orig_id);
}  // namespace clp::clp

#endif  // CLP_CLP_IRDECOMPRESSION_HPP
