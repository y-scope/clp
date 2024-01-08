#include "CommandLineArguments.hpp"

#include <iostream>

#include <boost/program_options.hpp>

#include "../clp/spdlog_with_specializations.hpp"

namespace po = boost::program_options;

namespace reducer {
clp::CommandLineArgumentsBase::ParsingResult
CommandLineArguments::parse_arguments(int argc, char const* argv[]) {
    try {
        po::options_description options("Reducer Options");
        options.add_options()(
            "reducer-host",
            po::value<std::string>(&m_reducer_host)
                ->default_value(m_reducer_host),
            "Host IP address of the server this reducer is running on"
        )(
            "reducer-port",
            po::value<int64_t>(&m_reducer_port)
                ->default_value(m_reducer_port),
            "Port this reducer should listen on for connections"
        )(
            "db-host",
            po::value<std::string>(&m_db_host)
                ->default_value(m_db_host),
            "Host the jobs database is running on"
        )(
            "db-port",
            po::value<int64_t>(&m_db_port)
                ->default_value(m_db_port),
            "Port the jobs database is listening on"
        )(
            "db-password",
            po::value<std::string>(&m_db_password)
                ->default_value(m_db_password),
            "Password for the jobs database"
        )(
            "db-database",
            po::value<std::string>(&m_db_database)
                ->default_value(m_db_database),
            "Database containing the jobs table"
        )(
            "mongodb-database",
            po::value<std::string>(&m_mongodb_database)
                ->default_value(m_mongodb_database),
            "MongoDB database for results"
        )(
            "mongodb-uri",
            po::value<std::string>(&m_mongodb_uri)
                ->default_value(m_mongodb_uri),
            "URI pointing to MongoDB database"
        )(
            "mongodb-metrics-collection",
            po::value<std::string>(&m_mongodb_jobs_metric_collection)
                ->default_value(m_mongodb_jobs_metric_collection),
            "MongoDB metrics collection name"
        )(
            "polling-interval-ms",
            po::value<int>(&m_polling_interval_ms)
                ->default_value(m_polling_interval_ms),
            "Polling interval for the jobs table in milliseconds"
        );

        po::variables_map parsed_command_line_options;
        po::store(po::parse_command_line(argc, argv, options), parsed_command_line_options);
        po::notify(parsed_command_line_options);
    } catch (std::exception& e) {
        SPDLOG_ERROR("Failed to parse command line arguments - {}", e.what());
        return clp::CommandLineArgumentsBase::ParsingResult::Failure;
    }
    return clp::CommandLineArgumentsBase::ParsingResult::Success;
}

void CommandLineArguments::print_basic_usage() const {
    std::cerr << "Usage: [OPTIONS]" << std::endl;
}
}  // namespace reducer
