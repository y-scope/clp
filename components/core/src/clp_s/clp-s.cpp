#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include <json/single_include/nlohmann/json.hpp>
#include <mongocxx/instance.hpp>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

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
#include "search/Expression.hpp"
#include "search/kql/kql.hpp"
#include "search/NarrowTypes.hpp"
#include "search/OrOfAndForm.hpp"
#include "search/Output.hpp"
#include "search/OutputHandler.hpp"
#include "search/SchemaMatch.hpp"
#include "TimestampPattern.hpp"
#include "TraceableException.hpp"
#include "Utils.hpp"

using namespace clp_s::search;
using clp_s::cArchiveFormatDevelopmentVersionFlag;
using clp_s::cEpochTimeMax;
using clp_s::cEpochTimeMin;
using clp_s::CommandLineArguments;

namespace {
/**
 * Compresses the input files specified by the command line arguments into an archive.
 * @param command_line_arguments
 * @return Whether compression was successful
 */
bool compress(CommandLineArguments const& command_line_arguments);

/**
 * Decompresses the archive specified by the given JsonConstructorOption.
 * @param json_constructor_option
 */
void decompress_archive(clp_s::JsonConstructorOption const& json_constructor_option);

/**
 * Searches the given archive.
 * @param command_line_arguments
 * @param archive_reader
 * @param expr A copy of the search AST which may be modified
 * @return Whether the search succeeded
 */
bool search_archive(
        CommandLineArguments const& command_line_arguments,
        const std::shared_ptr<clp_s::ArchiveReader>& archive_reader,
        std::shared_ptr<Expression> expr
);

bool compress(CommandLineArguments const& command_line_arguments) {
    auto archives_dir = std::filesystem::path(command_line_arguments.get_archives_dir());

    // Create output directory in case it doesn't exist
    try {
        std::filesystem::create_directory(archives_dir.string());
    } catch (std::exception& e) {
        SPDLOG_ERROR(
                "Failed to create archives directory {} - {}",
                archives_dir.string(),
                e.what()
        );
        return false;
    }

    clp_s::JsonParserOption option;
    option.file_paths = command_line_arguments.get_file_paths();
    option.archives_dir = archives_dir.string();
    option.target_encoded_size = command_line_arguments.get_target_encoded_size();
    option.compression_level = command_line_arguments.get_compression_level();
    option.timestamp_key = command_line_arguments.get_timestamp_key();
    option.print_archive_stats = command_line_arguments.print_archive_stats();

    auto const& db_config_container = command_line_arguments.get_metadata_db_config();
    if (db_config_container.has_value()) {
        auto const& db_config = db_config_container.value();
        auto metadata_db = std::make_shared<clp::GlobalMySQLMetadataDB>(
                db_config.get_metadata_db_host(),
                db_config.get_metadata_db_port(),
                db_config.get_metadata_db_username(),
                db_config.get_metadata_db_password(),
                db_config.get_metadata_db_name(),
                db_config.get_metadata_table_prefix()
        );
        option.metadata_db = std::move(metadata_db);
    } else {
        option.metadata_db = nullptr;
    }

    clp_s::JsonParser parser(option);
    parser.parse();
    parser.store();

    return true;
}

void decompress_archive(clp_s::JsonConstructorOption const& json_constructor_option) {
    clp_s::JsonConstructor constructor(json_constructor_option);
    constructor.construct();
    constructor.store();
}

bool search_archive(
        CommandLineArguments const& command_line_arguments,
        const std::shared_ptr<clp_s::ArchiveReader>& archive_reader,
        std::shared_ptr<Expression> expr
) {
    auto const& query = command_line_arguments.get_query();

    auto timestamp_dict = archive_reader->read_timestamp_dictionary();
    AddTimestampConditions add_timestamp_conditions(
            timestamp_dict->get_authoritative_timestamp_tokenized_column(),
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
        return false;
    }

    OrOfAndForm standardize_pass;
    if (expr = standardize_pass.run(expr); std::dynamic_pointer_cast<EmptyExpr>(expr)) {
        SPDLOG_ERROR("Query '{}' is logically false", query);
        return false;
    }

    NarrowTypes narrow_pass;
    if (expr = narrow_pass.run(expr); std::dynamic_pointer_cast<EmptyExpr>(expr)) {
        SPDLOG_ERROR("Query '{}' is logically false", query);
        return false;
    }

    ConvertToExists convert_pass;
    if (expr = convert_pass.run(expr); std::dynamic_pointer_cast<EmptyExpr>(expr)) {
        SPDLOG_ERROR("Query '{}' is logically false", query);
        return false;
    }

    // skip decompressing the archive if we won't match based on
    // the timestamp index
    EvaluateTimestampIndex timestamp_index(timestamp_dict);
    if (clp_s::EvaluatedValue::False == timestamp_index.run(expr)) {
        SPDLOG_INFO("No matching timestamp ranges for query '{}'", query);
        return true;
    }

    // Narrow against schemas
    SchemaMatch match_pass(archive_reader->get_schema_tree(), archive_reader->get_schema_map());
    if (expr = match_pass.run(expr); std::dynamic_pointer_cast<EmptyExpr>(expr)) {
        SPDLOG_INFO("No matching schemas for query '{}'", query);
        return true;
    }

    std::unique_ptr<OutputHandler> output_handler;
    if (command_line_arguments.get_mongodb_enabled()) {
        output_handler = std::make_unique<ResultsCacheOutputHandler>(
                command_line_arguments.get_mongodb_uri(),
                command_line_arguments.get_mongodb_collection(),
                command_line_arguments.get_batch_size(),
                command_line_arguments.get_max_num_results()
        );
    } else if (command_line_arguments.get_network_destination_enabled()) {
        output_handler = std::make_unique<NetworkOutputHandler>(
                command_line_arguments.get_host(),
                command_line_arguments.get_port()
        );
    } else {
        output_handler = std::make_unique<StandardOutputHandler>();
    }

    // output result
    Output output(
            match_pass,
            expr,
            archive_reader,
            timestamp_dict,
            std::move(output_handler),
            command_line_arguments.get_ignore_case()
    );
    output.filter();

    return true;
}
}  // namespace

int main(int argc, char const* argv[]) {
    try {
        auto stderr_logger = spdlog::stderr_logger_st("stderr");
        spdlog::set_default_logger(stderr_logger);
        spdlog::set_pattern("%Y-%m-%dT%H:%M:%S.%e%z [%l] %v");
    } catch (std::exception& e) {
        // NOTE: We can't log an exception if the logger couldn't be constructed
        return 1;
    }

    clp_s::TimestampPattern::init();

    CommandLineArguments command_line_arguments("clp-s");
    auto parsing_result = command_line_arguments.parse_arguments(argc, argv);
    switch (parsing_result) {
        case CommandLineArguments::ParsingResult::Failure:
            return 1;
        case CommandLineArguments::ParsingResult::InfoCommand:
            return 0;
        case CommandLineArguments::ParsingResult::Success:
            // Continue processing
            break;
    }

    if (CommandLineArguments::Command::Compress == command_line_arguments.get_command()) {
        if (false == compress(command_line_arguments)) {
            return 1;
        }
    } else if (CommandLineArguments::Command::Extract == command_line_arguments.get_command()) {
        auto const& archives_dir = command_line_arguments.get_archives_dir();
        if (false == std::filesystem::is_directory(archives_dir)) {
            SPDLOG_ERROR("'{}' is not a directory.", archives_dir);
            return 1;
        }

        clp_s::JsonConstructorOption option;
        option.output_dir = command_line_arguments.get_output_dir();
        try {
            auto const& archive_id = command_line_arguments.get_archive_id();
            if (false == archive_id.empty()) {
                option.archives_dir = std::filesystem::path{archives_dir} / archive_id;
                decompress_archive(option);
            } else {
                for (auto const& entry : std::filesystem::directory_iterator(archives_dir)) {
                    if (false == entry.is_directory()) {
                        // Skip non-directories
                        continue;
                    }

                    option.archives_dir = entry.path();
                    decompress_archive(option);
                }
            }
        } catch (clp_s::TraceableException& e) {
            SPDLOG_ERROR("{}", e.what());
            return 1;
        }
    } else {
        mongocxx::instance const mongocxx_instance{};

        auto const& query = command_line_arguments.get_query();

        auto query_stream = std::istringstream(query);
        auto expr = kql::parse_kql_expression(query_stream);
        if (nullptr == expr) {
            return 1;
        }

        if (std::dynamic_pointer_cast<EmptyExpr>(expr)) {
            SPDLOG_ERROR("Query '{}' is logically false", query);
            return 1;
        }

        auto const& archives_dir = command_line_arguments.get_archives_dir();
        if (false == std::filesystem::is_directory(archives_dir)) {
            SPDLOG_ERROR("'{}' is not a directory.", archives_dir);
            return 1;
        }

        auto const& archive_id = command_line_arguments.get_archive_id();
        auto archive_reader = std::make_shared<clp_s::ArchiveReader>();
        if (false == archive_id.empty()) {
            std::filesystem::path const archives_dir_path{archives_dir};
            std::string const archive_path{archives_dir_path / archive_id};

            archive_reader->open(archive_path);
            if (false == search_archive(command_line_arguments, archive_reader, expr)) {
                return 1;
            }
            archive_reader->close();
        } else {
            for (auto const& entry : std::filesystem::directory_iterator(archives_dir)) {
                if (false == entry.is_directory()) {
                    // Skip non-directories
                    continue;
                }

                archive_reader->open(entry.path());
                if (false == search_archive(command_line_arguments, archive_reader, expr->copy())) {
                    return 1;
                }
                archive_reader->close();
            }
        }
    }

    return 0;
}
