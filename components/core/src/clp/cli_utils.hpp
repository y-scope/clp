#ifndef CLP_CLI_UTILS_HPP
#define CLP_CLI_UTILS_HPP

#include <vector>

#include <boost/program_options/option.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

namespace clp {
/**
 * Parses unrecognized options according to the given options description.
 * @param options_description
 * @param options Vector of options previously parsed by boost::program_options and which may
 * contain options that have the unrecognized flag set
 * @param parsed_options Returns any parsed options that were newly recognized
 */
void parse_unrecognized_options(
        boost::program_options::options_description const& options_description,
        std::vector<boost::program_options::option> const& options,
        boost::program_options::variables_map& parsed_options
);
}  // namespace clp

#endif  // CLP_CLI_UTILS_HPP
