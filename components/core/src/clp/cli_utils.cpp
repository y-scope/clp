#include "cli_utils.hpp"

#include <boost/program_options.hpp>

namespace clp {
void parse_unrecognized_options(
        boost::program_options::options_description const& options_description,
        std::vector<boost::program_options::option> const& options,
        boost::program_options::variables_map& parsed_options
) {
    auto unrecognized_options = boost::program_options::collect_unrecognized(
            options,
            boost::program_options::include_positional
    );
    unrecognized_options.erase(unrecognized_options.begin());
    boost::program_options::store(
            boost::program_options::command_line_parser(unrecognized_options)
                    .options(options_description)
                    .run(),
            parsed_options
    );
    notify(parsed_options);
}
}  // namespace clp
