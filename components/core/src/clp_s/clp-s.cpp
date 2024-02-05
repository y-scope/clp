#include <filesystem>
#include <iostream>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <json/single_include/nlohmann/json.hpp>
#include <spdlog/sinks/stdout_sinks.h>

#include "../clp/GlobalMySQLMetadataDB.hpp"
#include "CommandLineArguments.hpp"
#include "Defs.hpp"
#include "JsonConstructor.hpp"
#include "JsonParser.hpp"
#include "ReaderUtils.hpp"
#include "search/AddTimestampConditions.hpp"
#include "search/ConvertToExists.hpp"
#include "search/EmptyExpr.hpp"
#include "search/EvaluateTimestampIndex.hpp"
#include "search/kql/kql.hpp"
#include "search/NarrowTypes.hpp"
#include "search/OrOfAndForm.hpp"
#include "search/Output.hpp"
#include "search/OutputHandler.hpp"
#include "search/SchemaMatch.hpp"
#include "TimestampPattern.hpp"
#include "Utils.hpp"

using namespace clp_s::search;
using clp_s::cArchiveFormatDevelopmentVersionFlag;
using clp_s::cEpochTimeMax;
using clp_s::cEpochTimeMin;
using clp_s::CommandLineArguments;

int main(int argc, char const* argv[]) {
    try {
        auto stderr_logger = spdlog::stderr_logger_st("stderr");
        spdlog::set_default_logger(stderr_logger);
        spdlog::set_pattern("%Y-%m-%dT%H:%M:%S.%e%z [%l] %v");
    } catch (std::exception& e) {
        // NOTE: We can't log an exception if the logger couldn't be constructed
        return -1;
    }

    CommandLineArguments command_line_arguments("clp-s");
    auto parsing_result = command_line_arguments.parse_arguments(argc, argv);
    switch (parsing_result) {
        case CommandLineArguments::ParsingResult::Failure:
            return -1;
        case CommandLineArguments::ParsingResult::InfoCommand:
            return 0;
        case CommandLineArguments::ParsingResult::Success:
            // Continue processing
            break;
    }

    if (CommandLineArguments::Command::Compress == command_line_arguments.get_command()) {
        clp_s::TimestampPattern::init();

        boost::uuids::random_generator generator;
        auto archive_id = boost::uuids::to_string(generator());
        auto archives_dir = std::filesystem::path(command_line_arguments.get_archives_dir());
        auto archive_path = archives_dir / archive_id;

        // Create output directory in case it doesn't exist
        try {
            std::filesystem::create_directory(archives_dir.string());
        } catch (std::exception& e) {
            SPDLOG_ERROR(
                    "Failed to create archives directory {} - {}",
                    archives_dir.string(),
                    e.what()
            );
            return 1;
        }

        clp_s::JsonParserOption option;
        option.file_paths = command_line_arguments.get_file_paths();
        option.archives_dir = archive_path.string();
        option.target_encoded_size = command_line_arguments.get_target_encoded_size();
        option.compression_level = command_line_arguments.get_compression_level();
        option.timestamp_key = command_line_arguments.get_timestamp_key();

        clp_s::JsonParser parser(option);
        parser.parse();
        parser.store();
        parser.close();

        if (command_line_arguments.print_archive_stats()) {
            nlohmann::json json_msg;
            json_msg["id"] = archive_id;
            json_msg["uncompressed_size"] = parser.get_uncompressed_size();
            json_msg["size"] = parser.get_compressed_size();
            std::cout << json_msg.dump(-1, ' ', true, nlohmann::json::error_handler_t::ignore)
                      << std::endl;
        }

        if (command_line_arguments.get_metadata_db_config().has_value()) {
            auto const& db_config = command_line_arguments.get_metadata_db_config().value();
            clp::GlobalMySQLMetadataDB metadata_db(
                    db_config.get_metadata_db_host(),
                    db_config.get_metadata_db_port(),
                    db_config.get_metadata_db_username(),
                    db_config.get_metadata_db_password(),
                    db_config.get_metadata_db_name(),
                    db_config.get_metadata_table_prefix()
            );

            clp::streaming_archive::ArchiveMetadata metadata(
                    cArchiveFormatDevelopmentVersionFlag,
                    "",
                    0ULL
            );
            metadata.increment_static_compressed_size(parser.get_compressed_size());
            metadata.increment_static_uncompressed_size(parser.get_uncompressed_size());
            metadata.expand_time_range(parser.get_begin_timestamp(), parser.get_end_timestamp());
            metadata_db.open();
            metadata_db.add_archive(archive_id, metadata);
            metadata_db.close();
        }
    } else if (CommandLineArguments::Command::Extract == command_line_arguments.get_command()) {
        clp_s::JsonConstructorOption option;
        option.archives_dir = command_line_arguments.get_archives_dir();
        option.output_dir = command_line_arguments.get_output_dir();

        clp_s::JsonConstructor constructor(option);
        constructor.construct();
        constructor.store();
        constructor.close();
    } else {
        auto const& archives_dir = command_line_arguments.get_archives_dir();
        auto const& query = command_line_arguments.get_query();
        clp_s::TimestampPattern::init();

        auto query_stream = std::istringstream(query);
        auto expr = kql::parse_kql_expression(query_stream);

        if (std::dynamic_pointer_cast<EmptyExpr>(expr)) {
            SPDLOG_ERROR("Query '{}' is logically false", query);
            return 1;
        }

        auto timestamp_dict = clp_s::ReaderUtils::read_timestamp_dictionary(archives_dir);
        AddTimestampConditions add_timestamp_conditions(
                timestamp_dict->get_authoritative_timestamp_column(),
                command_line_arguments.get_search_begin_ts(),
                command_line_arguments.get_search_end_ts()
        );
        if (expr = add_timestamp_conditions.run(expr); std::dynamic_pointer_cast<EmptyExpr>(expr)) {
            SPDLOG_ERROR(
                    "Query '{}' specified timestamp filters tge {} tle {}, but no authoritative "
                    "timestamp column was found for this archive",
                    query,
                    command_line_arguments.get_search_begin_ts().value_or(cEpochTimeMin),
                    command_line_arguments.get_search_end_ts().value_or(cEpochTimeMax)
            );
            return 1;
        }

        OrOfAndForm standardize_pass;
        if (expr = standardize_pass.run(expr); std::dynamic_pointer_cast<EmptyExpr>(expr)) {
            SPDLOG_ERROR("Query '{}' is logically false", query);
            return 1;
        }

        NarrowTypes narrow_pass;
        if (expr = narrow_pass.run(expr); std::dynamic_pointer_cast<EmptyExpr>(expr)) {
            SPDLOG_ERROR("Query '{}' is logically false", query);
            return 1;
        }

        ConvertToExists convert_pass;
        if (expr = convert_pass.run(expr); std::dynamic_pointer_cast<EmptyExpr>(expr)) {
            SPDLOG_ERROR("Query '{}' is logically false", query);
            return 1;
        }

        // skip decompressing the archive if we won't match based on
        // the timestamp index
        EvaluateTimestampIndex timestamp_index(timestamp_dict);
        if (clp_s::EvaluatedValue::False == timestamp_index.run(expr)) {
            SPDLOG_ERROR("No matching timestamp ranges for query '{}'", query);
            return 1;
        }

        auto schema_tree = clp_s::ReaderUtils::read_schema_tree(archives_dir);
        auto schemas = clp_s::ReaderUtils::read_schemas(archives_dir);

        // Narrow against schemas
        SchemaMatch match_pass(schema_tree, schemas);
        if (expr = match_pass.run(expr); std::dynamic_pointer_cast<EmptyExpr>(expr)) {
            SPDLOG_ERROR("No matching schemas for query '{}'", query);
            return 1;
        }

        std::unique_ptr<OutputHandler> output_handler;
        mongocxx::instance mongocxx_instance{};
        if (command_line_arguments.get_mongodb_enabled()) {
            output_handler = std::make_unique<ResultsCacheOutputHandler>(
                    command_line_arguments.get_mongodb_uri(),
                    command_line_arguments.get_mongodb_collection(),
                    command_line_arguments.get_batch_size()
            );
        } else {
            output_handler = std::make_unique<StandardOutputHandler>();
        }

        // output result
        Output output(
                schema_tree,
                schemas,
                match_pass,
                expr,
                archives_dir,
                timestamp_dict,
                std::move(output_handler)
        );
        output.filter();
    }

    return 0;
}
