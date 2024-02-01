#include <spdlog/sinks/stdout_sinks.h>

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

        clp_s::JsonParserOption option;
        option.file_paths = command_line_arguments.get_file_paths();
        option.archives_dir = command_line_arguments.get_archives_dir();
        option.target_encoded_size = command_line_arguments.get_target_encoded_size();
        option.compression_level = command_line_arguments.get_compression_level();
        option.timestamp_key = command_line_arguments.get_timestamp_key();

        clp_s::JsonParser parser(option);
        parser.parse();
        parser.store();
        parser.close();
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
