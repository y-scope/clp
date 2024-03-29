#ifndef GLT_GLT_SEARCH_HPP
#define GLT_GLT_SEARCH_HPP

#include "CommandLineArguments.hpp"

namespace glt::glt {
/**
 * perform search based on the command line input
 * @param command_line_args
 * @return true if search was successful, false otherwise
 */
bool search(CommandLineArguments& command_line_args);
}  // namespace glt::glt

#endif  // GLT_GLT_SEARCH_HPP
