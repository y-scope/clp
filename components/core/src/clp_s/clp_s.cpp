#include <spdlog/sinks/stdout_sinks.h>

#include "CommandLineArguments.hpp"
#include "JsonConstructor.hpp"
#include "JsonParser.hpp"
#include "ReaderUtils.hpp"
#include "search/ConvertToExists.hpp"
#include "search/EmptyExpr.hpp"
#include "search/EvaluateTimestampIndex.hpp"
#include "search/kql/KQL.hpp"
#include "search/NarrowTypes.hpp"
#include "search/OrOfAndForm.hpp"
#include "search/Output.hpp"
#include "search/SchemaMatch.hpp"
#include "TimestampPattern.hpp"
#include "Utils.hpp"

using namespace clp_s::search;
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
        option.archive_dir = command_line_arguments.get_archive_dir();
        option.max_encoding_size = command_line_arguments.get_max_encoding_size();
        option.compression_level = command_line_arguments.get_compression_level();
        auto const& timestamp_key = command_line_arguments.get_timestamp_key();
        if (false == timestamp_key.empty()) {
            clp_s::StringUtils::tokenize_column_descriptor(timestamp_key, option.timestamp_column);
        }

        clp_s::JsonParser parser(option);
        parser.parse();
        parser.store();
        parser.close();
    } else if (CommandLineArguments::Command::Extract == command_line_arguments.get_command()) {
        clp_s::JsonConstructorOption option;
        option.archive_dir = command_line_arguments.get_archive_dir();
        option.output_dir = command_line_arguments.get_output_dir();

        clp_s::JsonConstructor constructor(option);
        constructor.construct();
        constructor.store();
        constructor.close();
    } else {
        auto const& archive_dir = command_line_arguments.get_archive_dir();
        auto const& query = command_line_arguments.get_query();
        clp_s::TimestampPattern::init();

        auto query_stream = std::istringstream(query);
        auto expr = kql::parse_kql_expression(query_stream);

        if (std::dynamic_pointer_cast<EmptyExpr>(expr)) {
            SPDLOG_ERROR("Query '{}' is logically false", query);
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
        auto timestamp_dict = clp_s::ReaderUtils::read_timestamp_dictionary(archive_dir);
        EvaluateTimestampIndex timestamp_index(timestamp_dict);
        if (clp_s::EvaluatedValue::False == timestamp_index.run(expr)) {
            SPDLOG_ERROR("No matching timestamp ranges for query '{}'", query);
            return 1;
        }

        auto schema_tree = clp_s::ReaderUtils::read_schema_tree(archive_dir);
        auto schemas = clp_s::ReaderUtils::read_schemas(archive_dir);

        // Narrow against schemas
        SchemaMatch match_pass(schema_tree, schemas);
        if (expr = match_pass.run(expr); std::dynamic_pointer_cast<EmptyExpr>(expr)) {
            SPDLOG_ERROR("No matching schemas for query '{}'", query);
            return 1;
        }

        // output result
        Output output(schema_tree, schemas, match_pass, expr, archive_dir, timestamp_dict);
        output.filter();
    }

    return 0;
}
