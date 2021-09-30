#ifndef CLP_DECOMPRESSION_HPP
#define CLP_DECOMPRESSION_HPP

// C++ standard libraries
#include <string>
#include <unordered_set>

// Project headers
#include "CommandLineArguments.hpp"

namespace clp {
    /**
     * Decompresses an archive into the given directory
     * @param command_line_args
     * @param files_to_decompress
     * @return true if decompression was successful, false otherwise
     */
    bool decompress (CommandLineArguments& command_line_args, const std::unordered_set<std::string>& files_to_decompress);
}

#endif // CLP_DECOMPRESSION_HPP
