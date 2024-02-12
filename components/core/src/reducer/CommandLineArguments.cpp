#include "CommandLineArguments.hpp"

#include <iostream>

#include <boost/program_options.hpp>

#include "../clp/spdlog_with_specializations.hpp"

namespace po = boost::program_options;

namespace reducer {
clp::CommandLineArgumentsBase::ParsingResult
CommandLineArguments::parse_arguments(int argc, char const* argv[]) {
    try {
        po::options_description options_general("General Options");
        options_general.add_options()("help,h", "Print help");

        po::options_description options_reducer("Reducer Options");
        options_reducer.add_options()(
            "reducer-host",
            po::value<std::string>(&m_reducer_host)
                ->default_value(m_reducer_host),
            "Host that this reducer should bind to"
        )(
            "reducer-port",
            po::value<int>(&m_reducer_port)
                ->default_value(m_reducer_port),
            "Port this reducer should listen on for connections"
        )(
            "scheduler-host",
            po::value<std::string>(&m_scheduler_host)
                ->default_value(m_scheduler_host),
            "Host the search scheduler is running on"
        )(
            "scheduler-port",
            po::value<int>(&m_scheduler_port)
                ->default_value(m_scheduler_port),
            "Port the search scheduler is listening on"
        )(
            "mongodb-uri",
            po::value<std::string>(&m_mongodb_uri)
                ->default_value(m_mongodb_uri),
            "URI pointing to MongoDB database"
        )(
            "polling-interval-ms",
            po::value<int>(&m_polling_interval_ms)
                ->default_value(m_polling_interval_ms),
            "Polling interval for the jobs table in milliseconds"
        );

        po::options_description all_options;
        all_options.add(options_general);
        all_options.add(options_reducer);

        po::variables_map parsed_command_line_options;
        po::store(po::parse_command_line(argc, argv, all_options), parsed_command_line_options);
        po::notify(parsed_command_line_options);

        if (parsed_command_line_options.count("help")) {
            if (argc > 2) {
                SPDLOG_WARN("Ignoring all options besides --help.");
            }

            print_basic_usage();
            std::cerr << std::endl;
            std::cerr << "Options can be specified on the command line or through a configuration "
                      << "file." << std::endl;
            std::cerr << all_options << std::endl;
            return clp::CommandLineArgumentsBase::ParsingResult::InfoCommand;
        }
    } catch (std::exception& e) {
        SPDLOG_ERROR("Failed to parse command line arguments - {}", e.what());
        return clp::CommandLineArgumentsBase::ParsingResult::Failure;
    }

    // Validate arguments. Note: mysql username and password are allowed to be the empty string.
    bool valid_arguments = true;
    if (m_reducer_host.empty()) {
        SPDLOG_ERROR("Empty reducer-host argument");
        valid_arguments = false;
    }

    if (m_reducer_port <= 0) {
        SPDLOG_ERROR("Invalid argument for reducer-port {}", m_reducer_port);
        valid_arguments = false;
    }

    if (m_scheduler_host.empty()) {
        SPDLOG_ERROR("Empty scheduler-host argument");
        valid_arguments = false;
    }

    if (m_scheduler_port <= 0) {
        SPDLOG_ERROR("Invalid argument for scheduler-port {}", m_scheduler_port);
        valid_arguments = false;
    }

    if (m_mongodb_uri.empty()) {
        SPDLOG_ERROR("Empty mongodb-uri argument");
        valid_arguments = false;
    }

    if (m_polling_interval_ms <= 0) {
        SPDLOG_ERROR("Invalid argument for polling-interval-ms {}", m_polling_interval_ms);
        valid_arguments = false;
    }

    if (!valid_arguments) {
        return clp::CommandLineArgumentsBase::ParsingResult::Failure;
    }

    return clp::CommandLineArgumentsBase::ParsingResult::Success;
}

void CommandLineArguments::print_basic_usage() const {
    std::cerr << "Usage: " << get_program_name() << " [OPTIONS]" << std::endl;
}
}  // namespace reducer
