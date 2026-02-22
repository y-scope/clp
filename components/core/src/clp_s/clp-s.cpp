#include <cstdlib>
#include <exception>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

#include <fmt/format.h>
#include <mongocxx/instance.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ErrorCode.hpp>

#include "../clp/CurlGlobalInstance.hpp"
#include "../clp/ir/constants.hpp"
#include "../clp/streaming_archive/ArchiveMetadata.hpp"
#include "../reducer/network_utils.hpp"
#include "CommandLineArguments.hpp"
#include "Defs.h"
#include "Defs.hpp"
#include "JsonConstructor.hpp"
#include "JsonParser.hpp"
#include "kv_ir_search.hpp"
#include "OutputHandlerImpl.hpp"
#include "search/AddTimestampConditions.hpp"
#include "search/ast/ConvertToExists.hpp"
#include "search/ast/EmptyExpr.hpp"
#include "search/ast/Expression.hpp"
#include "search/ast/NarrowTypes.hpp"
#include "search/ast/OrOfAndForm.hpp"
#include "search/ast/SearchUtils.hpp"
#include "search/ast/SetTimestampLiteralPrecision.hpp"
#include "search/ast/TimestampLiteral.hpp"
#include "search/EvaluateRangeIndexFilters.hpp"
#include "search/EvaluateTimestampIndex.hpp"
#include "search/kql/kql.hpp"
#include "search/Output.hpp"
#include "search/OutputHandler.hpp"
#include "search/Projection.hpp"
#include "search/SchemaMatch.hpp"
#include "TimestampPattern.hpp"

using namespace clp_s::search;
using clp_s::cArchiveFormatDevelopmentVersionFlag;
using clp_s::cEpochTimeMax;
using clp_s::cEpochTimeMin;
using clp_s::CommandLineArguments;
using clp_s::KvIrSearchError;
using clp_s::KvIrSearchErrorEnum;

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
        std::shared_ptr<clp_s::ArchiveReader> const& archive_reader,
        std::shared_ptr<ast::Expression> expr
);

/**
 * @return -1 if no experimental query found, 0 on success, >0 on failure
 */
auto handle_experimental_queries(CommandLineArguments const& cli_args) -> int;

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

    clp_s::JsonParserOption option{};
    option.input_paths = command_line_arguments.get_input_paths();
    option.network_auth = command_line_arguments.get_network_auth();
    option.archives_dir = archives_dir.string();
    option.target_encoded_size = command_line_arguments.get_target_encoded_size();
    option.max_document_size = command_line_arguments.get_max_document_size();
    option.min_table_size = command_line_arguments.get_minimum_table_size();
    option.compression_level = command_line_arguments.get_compression_level();
    option.timestamp_key = command_line_arguments.get_timestamp_key();
    option.print_archive_stats = command_line_arguments.print_archive_stats();
    option.retain_float_format = command_line_arguments.get_retain_float_format();
    option.single_file_archive = command_line_arguments.get_single_file_archive();
    option.structurize_arrays = command_line_arguments.get_structurize_arrays();
    option.record_log_order = command_line_arguments.get_record_log_order();
    option.experimental = command_line_arguments.experimental();
    option.log_surgeon_schema_path = command_line_arguments.get_log_surgeon_schema_path();

    clp_s::JsonParser parser(option);
    if (false == parser.ingest()) {
        SPDLOG_ERROR("Encountered error while parsing input.");
        return false;
    }
    std::ignore = parser.store();
    return true;
}

void decompress_archive(clp_s::JsonConstructorOption const& json_constructor_option) {
    clp_s::JsonConstructor constructor(json_constructor_option);
    constructor.store();
}

bool search_archive(
        CommandLineArguments const& command_line_arguments,
        std::shared_ptr<clp_s::ArchiveReader> const& archive_reader,
        std::shared_ptr<ast::Expression> expr
) {
    auto const& query = command_line_arguments.get_query();

    auto timestamp_dict = archive_reader->get_timestamp_dictionary();
    AddTimestampConditions add_timestamp_conditions(
            timestamp_dict->get_authoritative_timestamp_tokenized_column(),
            command_line_arguments.get_search_begin_ts(),
            command_line_arguments.get_search_end_ts()
    );
    if (expr = add_timestamp_conditions.run(expr); std::dynamic_pointer_cast<ast::EmptyExpr>(expr))
    {
        SPDLOG_ERROR(
                "Query '{}' specified timestamp filters tge {} tle {}, but no authoritative "
                "timestamp column was found for this archive",
                query,
                command_line_arguments.get_search_begin_ts().value_or(cEpochTimeMin),
                command_line_arguments.get_search_end_ts().value_or(cEpochTimeMax)
        );
        return false;
    }

    ast::OrOfAndForm standardize_pass;
    if (expr = standardize_pass.run(expr); std::dynamic_pointer_cast<ast::EmptyExpr>(expr)) {
        SPDLOG_ERROR("Query '{}' is logically false", query);
        return false;
    }

    ast::NarrowTypes narrow_pass;
    if (expr = narrow_pass.run(expr); std::dynamic_pointer_cast<ast::EmptyExpr>(expr)) {
        SPDLOG_ERROR("Query '{}' is logically false", query);
        return false;
    }

    ast::ConvertToExists convert_pass;
    if (expr = convert_pass.run(expr); std::dynamic_pointer_cast<ast::EmptyExpr>(expr)) {
        SPDLOG_ERROR("Query '{}' is logically false", query);
        return false;
    }

    EvaluateRangeIndexFilters metadata_filter_pass{
            archive_reader->get_range_index(),
            false == command_line_arguments.get_ignore_case()
    };
    if (expr = metadata_filter_pass.run(expr); std::dynamic_pointer_cast<ast::EmptyExpr>(expr)) {
        SPDLOG_INFO("No matching metadata ranges for query '{}'", query);
        return true;
    }

    // skip decompressing the archive if we won't match based on
    // the timestamp index
    EvaluateTimestampIndex timestamp_index(timestamp_dict);
    if (clp_s::EvaluatedValue::False == timestamp_index.run(expr)) {
        SPDLOG_INFO("No matching timestamp ranges for query '{}'", query);
        return true;
    }

    ast::SetTimestampLiteralPrecision date_precision_pass{
            ast::TimestampLiteral::Precision::Milliseconds
    };
    expr = date_precision_pass.run(expr);

    // Narrow against schemas
    auto match_pass = std::make_shared<SchemaMatch>(
            archive_reader->get_schema_tree(),
            archive_reader->get_schema_map()
    );
    if (expr = match_pass->run(expr); std::dynamic_pointer_cast<ast::EmptyExpr>(expr)) {
        SPDLOG_INFO("No matching schemas for query '{}'", query);
        return true;
    }

    // Populate projection
    auto projection = std::make_shared<Projection>(
            command_line_arguments.get_projection_columns().empty()
                    ? ProjectionMode::ReturnAllColumns
                    : ProjectionMode::ReturnSelectedColumns
    );
    try {
        for (auto const& column : command_line_arguments.get_projection_columns()) {
            std::vector<std::string> descriptor_tokens;
            std::string descriptor_namespace;
            if (false
                == clp_s::search::ast::tokenize_column_descriptor(
                        column,
                        descriptor_tokens,
                        descriptor_namespace
                ))
            {
                SPDLOG_ERROR("Can not tokenize invalid column: \"{}\"", column);
                return false;
            }
            projection->add_column(
                    ast::ColumnDescriptor::create_from_escaped_tokens(
                            descriptor_tokens,
                            descriptor_namespace
                    )
            );
        }
    } catch (std::exception const& e) {
        SPDLOG_ERROR("{}", e.what());
        return false;
    }
    projection->resolve_columns(archive_reader->get_schema_tree());
    archive_reader->set_projection(projection);

    auto output_handler{command_line_arguments.create_output_handler()};
    if (output_handler.has_error()) {
        return false;
    }

    // output result
    Output output(
            match_pass,
            expr,
            archive_reader,
            std::move(output_handler.value()),
            command_line_arguments.get_ignore_case()
    );
    return output.filter();
}

auto handle_experimental_queries(CommandLineArguments const& cli_args) -> int {
    auto const& query = cli_args.get_query();
    if (CommandLineArguments::ExperimentalQueries::cLogTypeStatsQuery != query
        && CommandLineArguments::ExperimentalQueries::cVariableStatsQuery != query)
    {
        return -1;
    }
    auto output_handler{cli_args.create_output_handler()};
    if (output_handler.has_error()) {
        SPDLOG_ERROR("Failed to create output handler - {}", output_handler.error().message());
        return 1;
    }
    auto archive_reader = std::make_shared<clp_s::ArchiveReader>();
    for (auto const& input_path : cli_args.get_input_paths()) {
        try {
            archive_reader->open(
                    input_path,
                    clp_s::ArchiveReader::Options{
                            cli_args.get_network_auth(),
                            cli_args.experimental()
                    }
            );
        } catch (std::exception const& e) {
            SPDLOG_ERROR("Failed to open archive - {}", e.what());
            return 2;
        }
        archive_reader->read_dictionaries_and_metadata();
        auto const stats{archive_reader->get_experimental_stats()};
        if (false == stats.has_value()) {
            SPDLOG_ERROR("Failed to get experimental stats");
            return 3;
        }
        if (CommandLineArguments::ExperimentalQueries::cLogTypeStatsQuery == query) {
            auto logtype_dict{archive_reader->get_log_type_dictionary()};
            for (clp::logtype_dictionary_id_t i{0}; i < stats->m_logtype_stats.size(); ++i) {
                auto logtype{clp_s::ArchiveReader::decode_logtype_with_variable_types(
                        logtype_dict->get_entry(i),
                        stats->m_logtype_stats
                )};
                if (logtype.has_error()) {
                    SPDLOG_ERROR(
                            "Failed to get log type for ID {}: {}",
                            i,
                            logtype.error().message()
                    );
                    return 4;
                }
                auto message{fmt::format(
                        "{{\"id\":{},\"count\":{},\"log_type\":\"{}\"}}\n",
                        i,
                        stats->m_logtype_stats.at(i).get_count(),
                        logtype.value()
                )};
                output_handler.value()->write(message);
            }
        } else if (CommandLineArguments::ExperimentalQueries::cVariableStatsQuery == query) {
            // auto var_dict{archive_reader->get_variable_dictionary()};
            // for (clp::variable_dictionary_id_t i{0}; i < stats->m_var_stats.size(); ++i) {
            //     auto var_stat{stats->m_var_stats.at(i)};
            //     if (0 == var_stat.get_count()) {
            //         continue;
            //     }
            //     auto message{fmt::format(
            //             "{{\"id\":{},\"count\":{},\"type\":\"{}\",\"variable\":\"{}\"}}\n",
            //             i,
            //             var_stat.get_count(),
            //             var_stat.get_type(),
            //             var_dict->get_value(i)
            //     )};
            //     output_handler.value()->write(message);
            // }
        }
        if (auto ec{output_handler.value()->flush()}; clp_s::ErrorCode::ErrorCodeSuccess != ec) {
            SPDLOG_ERROR("Failed to flush output handler. Error code: {}", std::to_string(ec));
            return 5;
        }
        archive_reader->close();
    }
    return 0;
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
    mongocxx::instance const mongocxx_instance{};
    clp::CurlGlobalInstance const curl_instance{};

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
        try {
            if (false == compress(command_line_arguments)) {
                return 1;
            }
        } catch (std::exception const& e) {
            SPDLOG_ERROR("Encountered error during compression - {}", e.what());
            return 1;
        }
    } else if (CommandLineArguments::Command::Extract == command_line_arguments.get_command()) {
        clp_s::JsonConstructorOption option{};
        option.output_dir = command_line_arguments.get_output_dir();
        option.ordered = command_line_arguments.get_ordered_decompression();
        option.target_ordered_chunk_size = command_line_arguments.get_target_ordered_chunk_size();
        option.print_ordered_chunk_stats = command_line_arguments.print_ordered_chunk_stats();
        option.network_auth = command_line_arguments.get_network_auth();
        if (false == command_line_arguments.get_mongodb_uri().empty()) {
            option.metadata_db
                    = {command_line_arguments.get_mongodb_uri(),
                       command_line_arguments.get_mongodb_collection()};
        }

        try {
            for (auto const& archive_path : command_line_arguments.get_input_paths()) {
                option.archive_path = archive_path;
                decompress_archive(option);
            }
        } catch (std::exception const& e) {
            SPDLOG_ERROR("Encountered error during decompression - {}", e.what());
            return 1;
        }
    } else {
        auto const& query = command_line_arguments.get_query();
        if (auto const result{handle_experimental_queries(command_line_arguments)}; -1 < result) {
            return result;
        }

        auto query_stream = std::istringstream(query);
        auto expr = kql::parse_kql_expression(query_stream);
        if (nullptr == expr) {
            return 1;
        }

        if (std::dynamic_pointer_cast<ast::EmptyExpr>(expr)) {
            SPDLOG_ERROR("Query '{}' is logically false", query);
            return 1;
        }

        auto archive_reader = std::make_shared<clp_s::ArchiveReader>();
        for (auto const& input_path : command_line_arguments.get_input_paths()) {
            if (std::string::npos != input_path.path.find(clp::ir::cIrFileExtension)) {
                auto const result{
                        clp_s::search_kv_ir_stream(input_path, command_line_arguments, expr->copy())
                };
                if (false == result.has_error()) {
                    continue;
                }

                auto const error{result.error()};
                if (std::errc::result_out_of_range == error) {
                    // To support real-time search, we will allow incomplete IR streams.
                    // TODO: Use dedicated error code for this case once issue #904 is resolved.
                    SPDLOG_WARN("IR stream `{}` is truncated", input_path.path);
                    continue;
                }

                if (KvIrSearchError{KvIrSearchErrorEnum::ProjectionSupportNotImplemented} == error
                    || KvIrSearchError{KvIrSearchErrorEnum::UnsupportedOutputHandlerType} == error
                    || KvIrSearchError{KvIrSearchErrorEnum::CountSupportNotImplemented} == error)
                {
                    // These errors are treated as non-fatal because they result from unsupported
                    // features. However, this approach may cause archives with this extension to be
                    // skipped if the search uses advanced features that are not yet implemented. To
                    // mitigate this, we log a warning and proceed to search the input as an
                    // archive.
                    SPDLOG_WARN(
                            "Attempted to search an IR stream using unsupported features. Falling"
                            " back to searching the input as an archive."
                    );
                } else if (KvIrSearchError{KvIrSearchErrorEnum::DeserializerCreationFailure}
                           != error)
                {
                    // If the error is `DeserializerCreationFailure`, we may continue to treat the
                    // input as an archive and retry. Otherwise, it should be considered as a
                    // non-recoverable failure and return directly.
                    SPDLOG_ERROR(
                            "Failed to search '{}' as an IR stream, error_category={}, error={}",
                            input_path.path,
                            error.category().name(),
                            error.message()
                    );
                    return 1;
                }
            }

            try {
                archive_reader->open(
                        input_path,
                        clp_s::ArchiveReader::Options{
                                command_line_arguments.get_network_auth(),
                                command_line_arguments.experimental()
                        }
                );
            } catch (std::exception const& e) {
                SPDLOG_ERROR("Failed to open archive - {}", e.what());
                return 1;
            }
            if (false == search_archive(command_line_arguments, archive_reader, expr->copy())) {
                return 1;
            }
            archive_reader->close();
        }
    }

    return 0;
}
