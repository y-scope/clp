#include "CommandLineArguments.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "../ErrorCode.hpp"
#include "../FileReader.hpp"
#include "../InputConfig.hpp"

namespace po = boost::program_options;

namespace clp_s::log_converter {
namespace {
// Authorization method constants
constexpr std::string_view cNoAuth{"none"};
constexpr std::string_view cS3Auth{"s3"};

/**
 * Reads and returns a list of paths from a file containing newline-delimited paths.
 * @param input_path_list_file_path Path to the file containing the list of paths.
 * @param path_destination The vector that the paths are pushed into.
 * @return Whether paths were read successfully or not.
 */
[[nodiscard]] auto read_paths_from_file(
        std::string const& input_path_list_file_path,
        std::vector<std::string>& path_destination
) -> bool;

/**
 * Validates and populates network authorization options.
 * @param auth_method
 * @param auth
 * @throws std::invalid_argument if the authorization option is invalid
 */
void validate_network_auth(std::string_view auth_method, NetworkAuthOption& auth);

auto read_paths_from_file(
        std::string const& input_path_list_file_path,
        std::vector<std::string>& path_destination
) -> bool {
    FileReader reader;
    auto error_code = reader.try_open(input_path_list_file_path);
    if (ErrorCodeFileNotFound == error_code) {
        SPDLOG_ERROR(
                "Failed to open input path list file {} - file not found",
                input_path_list_file_path
        );
        return false;
    }
    if (ErrorCodeSuccess != error_code) {
        SPDLOG_ERROR("Error opening input path list file {}", input_path_list_file_path);
        return false;
    }

    std::string line;
    while (true) {
        error_code = reader.try_read_to_delimiter('\n', false, false, line);
        if (ErrorCodeSuccess != error_code) {
            break;
        }
        if (false == line.empty()) {
            path_destination.push_back(line);
        }
    }

    if (ErrorCodeEndOfFile != error_code) {
        return false;
    }
    return true;
}

void validate_network_auth(std::string_view auth_method, NetworkAuthOption& auth) {
    if (cS3Auth == auth_method) {
        auth.method = AuthMethod::S3PresignedUrlV4;
    } else if (cNoAuth != auth_method) {
        throw std::invalid_argument(fmt::format("Invalid authentication type \"{}\"", auth_method));
    }
}
}  // namespace

auto CommandLineArguments::parse_arguments(int argc, char const** argv)
        -> CommandLineArguments::ParsingResult {
    if (1 == argc) {
        print_basic_usage();
        return ParsingResult::Failure;
    }

    try {
        po::variables_map parsed_command_line_options;

        po::options_description general_options("General options");
        general_options.add_options()("help,h", "Print help");

        po::options_description conversion_positional_options;
        std::vector<std::string> input_paths;
        // clang-format off
        conversion_positional_options.add_options()(
                "input-paths",
                po::value<std::vector<std::string>>(&input_paths)->value_name("PATHS"),
                "input paths"
        );
        // clang-format on

        po::options_description conversion_options("Conversion options");
        std::string input_path_list_file_path;
        std::string auth{cNoAuth};
        // clang-format off
        conversion_options.add_options()(
                "inputs-from,f",
                po::value<std::string>(&input_path_list_file_path)
                        ->value_name("INPUTS_FILE")
                        ->default_value(input_path_list_file_path),
                "Convert inputs specified in INPUTS_FILE."
        )(
                "output-dir",
                po::value<std::string>(&m_output_dir)
                    ->value_name("OUTPUT_DIR")
                    ->default_value(m_output_dir),
                "Output directory for converted inputs."
        )(
                "auth",
                po::value<std::string>(&auth)
                    ->value_name("AUTH_METHOD")
                    ->default_value(auth),
                "Type of authentication required for network requests (s3 | none). Authentication"
                " with s3 requires the AWS_ACCESS_KEY_ID and AWS_SECRET_ACCESS_KEY environment"
                " variables, and optionally the AWS_SESSION_TOKEN environment variable."
        );
        // clang-format on

        po::positional_options_description positional_options;
        positional_options.add("input-paths", -1);

        po::options_description all_conversion_options;
        all_conversion_options.add(general_options);
        all_conversion_options.add(conversion_options);
        all_conversion_options.add(conversion_positional_options);

        po::store(
                po::command_line_parser(argc, argv)
                        .options(all_conversion_options)
                        .positional(positional_options)
                        .run(),
                parsed_command_line_options
        );
        po::notify(parsed_command_line_options);

        if (parsed_command_line_options.contains("help")) {
            if (argc > 2) {
                SPDLOG_WARN("Ignoring all options besides --help.");
            }

            print_basic_usage();
            po::options_description visible_options;
            visible_options.add(general_options);
            visible_options.add(conversion_options);
            std::cerr << visible_options << '\n';
            return ParsingResult::InfoCommand;
        }

        if (false == input_path_list_file_path.empty()) {
            if (false == read_paths_from_file(input_path_list_file_path, input_paths)) {
                SPDLOG_ERROR("Failed to read paths from {}", input_path_list_file_path);
                return ParsingResult::Failure;
            }
        }

        for (auto const& path : input_paths) {
            if (false == get_input_files_for_raw_path(path, m_input_paths)) {
                throw std::invalid_argument(fmt::format("Invalid input path \"{}\".", path));
            }
        }

        if (m_input_paths.empty()) {
            throw std::invalid_argument("No input paths specified.");
        }

        validate_network_auth(auth, m_network_auth);
    } catch (std::exception& e) {
        SPDLOG_ERROR("{}", e.what());
        print_basic_usage();
        std::cerr << "Try " << get_program_name() << " --help for detailed usage instructions\n";
        return ParsingResult::Failure;
    }

    return ParsingResult::Success;
}

void CommandLineArguments::print_basic_usage() const {
    std::cerr << "Usage: " << get_program_name() << " [INPUT_PATHS] [OPTIONS]\n";
}
}  // namespace clp_s::log_converter
