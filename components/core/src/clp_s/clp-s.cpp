#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

#include <fmt/format.h>
#include <mongocxx/instance.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include <string_utils/string_utils.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/AggregationSink.hpp>
#include <clp_s/aggregators.hpp>
#include <clp_s/ArchiveReader.hpp>
#include <clp_s/ErrorCode.hpp>
#include <clpp/Defs.hpp>
#include <clpp/ErrorCode.hpp>

#if CLP_BUILD_CLP_S_ENABLE_CURL
    #include "../clp/CurlGlobalInstance.hpp"
#endif
#include <clp/type_utils.hpp>
#include <clp_s/search/ast/ColumnDescriptor.hpp>
#include <clp_s/search/ast/FunctionCall.hpp>
#include <clp_s/search/SearchTelemetry.hpp>
#include <clp_s/search/TelemetryContext.hpp>

#include "../clp/ir/constants.hpp"
#include "../clp/streaming_archive/ArchiveMetadata.hpp"
#include "../reducer/network_utils.hpp"
#include "CommandLineArguments.hpp"
#include "Defs.hpp"
#include "JsonConstructor.hpp"
#include "JsonParser.hpp"
#include "kv_ir_search.hpp"
#include "OutputHandlerImpl.hpp"
#include "search/AddTimestampConditions.hpp"
#include "search/ast/EmptyExpr.hpp"
#include "search/ast/Expression.hpp"
#include "search/ast/SetTimestampLiteralPrecision.hpp"
#include "search/ast/TimestampLiteral.hpp"
#include "search/EvaluateRangeIndexFilters.hpp"
#include "search/EvaluateTimestampIndex.hpp"
#include "search/kql/kql.hpp"
#include "search/Output.hpp"
#include "search/OutputHandler.hpp"
#include "search/Projection.hpp"
#include "search/SchemaMatch.hpp"
#include "SingleFileArchiveDefs.hpp"

using namespace clp_s::search;
using clp_s::cArchiveFormatDevelopmentVersionFlag;
using clp_s::cEpochTimeMax;
using clp_s::cEpochTimeMin;
using clp_s::CommandLineArguments;
using clp_s::KvIrSearchError;
using clp_s::KvIrSearchErrorEnum;

namespace {
/**
 * Create the appropriate OutputHandler based on the cli arguments supplied.
 */
[[nodiscard]] auto
create_output_handler(CommandLineArguments const& cli_args, std::string_view archive_id)
        -> ystdlib::error_handling::Result<std::unique_ptr<OutputHandler>>;

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
 *
 * @param command_line_arguments
 * @param archive_reader
 * @param expr A copy of the search AST which may be modified.
 * @param telemetry_span The span to record search telemetry onto, or null if telemetry is disabled.
 * @return Whether the search succeeded.
 */
bool search_archive(
        CommandLineArguments const& command_line_arguments,
        std::shared_ptr<clp_s::ArchiveReader> const& archive_reader,
        std::shared_ptr<ast::Expression> expr,
        std::shared_ptr<SearchTelemetrySpan> const& telemetry_span
);

auto create_output_handler(CommandLineArguments const& cli_args, std::string_view archive_id)
        -> ystdlib::error_handling::Result<std::unique_ptr<OutputHandler>> {
    try {
        auto const& aggregator{cli_args.get_aggregator()};
        return std::visit(
                clp::overloaded{
                        [&](CommandLineArguments::FileOutputHandlerOptions const& options)
                                -> ystdlib::error_handling::Result<std::unique_ptr<OutputHandler>> {
                            return std::make_unique<clp_s::FileOutputHandler>(
                                    options.output_path,
                                    true
                            );
                        },
                        [&](CommandLineArguments::NetworkOutputHandlerOptions const& options)
                                -> ystdlib::error_handling::Result<std::unique_ptr<OutputHandler>> {
                            return std::make_unique<clp_s::NetworkOutputHandler>(
                                    options.host,
                                    options.port
                            );
                        },
                        [&](CommandLineArguments::ReducerOutputHandlerOptions const& options)
                                -> ystdlib::error_handling::Result<std::unique_ptr<OutputHandler>> {
                            auto const reducer_socket_fd{reducer::connect_to_reducer(
                                    options.host,
                                    options.port,
                                    options.job_id
                            )};
                            if (-1 == reducer_socket_fd) {
                                SPDLOG_ERROR("Failed to connect to reducer");
                                return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::BadParam};
                            }

                            if (false == aggregator.has_value()) {
                                SPDLOG_ERROR("Empty aggregation type.");
                                return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::BadParam};
                            }
                            if (std::holds_alternative<clp_s::CountAggregator>(
                                        aggregator.value()
                                )) {
                                return std::make_unique<clp_s::CountReducerOutputHandler>(
                                        reducer_socket_fd
                                );
                            }
                            if (std::holds_alternative<clp_s::CountByTimeAggregator>(
                                        aggregator.value()
                                )) {
                                return std::make_unique<clp_s::CountByTimeReducerOutputHandler>(
                                        reducer_socket_fd,
                                        std::get<clp_s::CountByTimeAggregator>(aggregator.value())
                                                .get_bucket_size_millisecs()
                                );
                            }
                            SPDLOG_ERROR("Unhandled aggregation type.");
                            return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::BadParam};
                        },
                        [&](CommandLineArguments::ResultsCacheOutputHandlerOptions const& options)
                                -> ystdlib::error_handling::Result<std::unique_ptr<OutputHandler>> {
                            if (false == aggregator.has_value()) {
                                return std::make_unique<clp_s::ResultsCacheOutputHandler>(
                                        options.uri,
                                        options.collection,
                                        options.batch_size,
                                        options.max_num_results,
                                        options.dataset
                                );
                            }
                            return clp_s::make_aggregation_output_handler(
                                    aggregator.value(),
                                    std::make_unique<clp_s::ResultsCacheSink>(
                                            options.uri,
                                            options.collection,
                                            archive_id
                                    )
                            );
                        },
                        [&](CommandLineArguments::StdoutOutputHandlerOptions const& /*options*/)
                                -> ystdlib::error_handling::Result<std::unique_ptr<OutputHandler>> {
                            if (false == aggregator.has_value()) {
                                return std::make_unique<clp_s::StandardOutputHandler>();
                            }
                            return clp_s::make_aggregation_output_handler(
                                    aggregator.value(),
                                    std::make_unique<clp_s::StdoutSink>(archive_id)
                            );
                        }

                },
                cli_args.get_output_handler_options()
        );
    } catch (std::exception const& e) {
        SPDLOG_ERROR("Failed to create output handler - {}", e.what());
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Failure};
    }
}

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
    option.input_paths_and_canonical_filenames
            = command_line_arguments.get_input_paths_and_canonical_filenames();
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
        std::shared_ptr<ast::Expression> expr,
        std::shared_ptr<SearchTelemetrySpan> const& telemetry_span
) {
    auto const& query = command_line_arguments.get_query();
    if (nullptr != telemetry_span) {
        telemetry_span->set_query_context(query);
        telemetry_span->set_archive_context(archive_reader->get_archive_id());
    }
    auto const record_error_and_log
            = [&](std::string_view error_message, std::string_view log_message) {
                  SPDLOG_ERROR("{}", log_message);
                  if (nullptr != telemetry_span) {
                      telemetry_span->set_error(error_message);
                  }
              };
    auto const record_early_termination = [&](std::string_view termination_stage) {
        if (nullptr == telemetry_span) {
            return;
        }
        try {
            if (auto const result{archive_reader->read_metadata()}; result.has_error()) {
                auto const error{result.error()};
                SPDLOG_WARN(
                        "Failed to read archive metadata for search telemetry - ({}) {}",
                        error.category().name(),
                        error.message()
                );
            }
        } catch (std::exception const& e) {
            SPDLOG_WARN("Failed to read archive metadata for search telemetry - {}", e.what());
        }
        SearchResultMetrics metrics;
        for (auto const schema_id : archive_reader->get_schema_ids()) {
            metrics.num_archive_records += archive_reader->get_num_messages_for_schema(schema_id);
        }
        telemetry_span->set_termination_stage(termination_stage);
        telemetry_span->set_search_result_metrics(metrics);
    };

    auto timestamp_dict = archive_reader->get_timestamp_dictionary();
    AddTimestampConditions add_timestamp_conditions(
            timestamp_dict->get_authoritative_timestamp_tokenized_column(),
            command_line_arguments.get_search_begin_ts(),
            command_line_arguments.get_search_end_ts()
    );
    if (expr = add_timestamp_conditions.run(expr); std::dynamic_pointer_cast<ast::EmptyExpr>(expr))
    {
        record_error_and_log(
                "no authoritative timestamp column",
                fmt::format(
                        "Query '{}' specified timestamp filters tge {} tle {}, but no authoritative"
                        " timestamp column was found for this archive",
                        query,
                        command_line_arguments.get_search_begin_ts().value_or(cEpochTimeMin),
                        command_line_arguments.get_search_end_ts().value_or(cEpochTimeMax)
                )
        );
        return false;
    }

    if (expr = clp_s::search::ast::preprocess_query(expr);
        std::dynamic_pointer_cast<ast::EmptyExpr>(expr))
    {
        record_error_and_log(
                "query is logically false",
                fmt::format("Query '{}' is logically false", query)
        );
        return false;
    }
    if (nullptr != telemetry_span) {
        telemetry_span->set_query_shape_metrics(
                QueryShapeMetrics::create(
                        expr,
                        command_line_arguments.get_search_begin_ts(),
                        command_line_arguments.get_search_end_ts()
                )
        );
    }

    EvaluateRangeIndexFilters metadata_filter_pass{
            archive_reader->get_range_index(),
            false == command_line_arguments.get_ignore_case()
    };
    if (expr = metadata_filter_pass.run(expr); std::dynamic_pointer_cast<ast::EmptyExpr>(expr)) {
        record_early_termination(cTerminationStageRangeIndexMatching);
        SPDLOG_INFO("No matching metadata ranges for query '{}'", query);
        return true;
    }

    // skip decompressing the archive if we won't match based on
    // the timestamp index
    EvaluateTimestampIndex timestamp_index(timestamp_dict);
    if (clp_s::EvaluatedValue::False == timestamp_index.run(expr)) {
        record_early_termination(cTerminationStageTimeRangeMatching);
        SPDLOG_INFO("No matching timestamp ranges for query '{}'", query);
        return true;
    }

    if (archive_reader->has_deprecated_timestamp_format()) {
        ast::SetTimestampLiteralPrecision date_precision_pass{
                ast::TimestampLiteral::Precision::Milliseconds
        };
        expr = date_precision_pass.run(expr);
    }

    // Narrow against schemas
    auto match_pass = std::make_shared<SchemaMatch>(archive_reader);
    if (expr = match_pass->run(expr); std::dynamic_pointer_cast<ast::EmptyExpr>(expr)) {
        record_early_termination(cTerminationStageSchemaMatching);
        SPDLOG_INFO("No matching schemas for query '{}'", query);
        return true;
    }

    // Populate projection
    auto projection = std::make_shared<Projection>(
            command_line_arguments.get_projection_columns().empty()
                    ? Projection::Mode::ReturnAllColumns
                    : Projection::Mode::ReturnSelectedColumns
    );
    try {
        for (auto const& column : command_line_arguments.get_projection_columns()) {
            auto parsed{kql::parse_projection_column(column)};
            if (nullptr == parsed) {
                record_error_and_log(
                        "parsing projection column failed",
                        fmt::format("Can not parse projection column: \"{}\"", column)
                );
                return false;
            }
            if (auto func_call{std::dynamic_pointer_cast<ast::FunctionCall>(parsed)}) {
                projection->add_column(func_call);
            } else if (auto col_desc{std::dynamic_pointer_cast<ast::ColumnDescriptor>(parsed)}) {
                projection->add_column(col_desc, Projection::OutputType::Default);
            } else {
                throw std::runtime_error{
                        fmt::format("Unexpected projection column type for: \"{}\"", column)
                };
            }
        }
        projection->resolve_columns(*archive_reader->get_schema_tree());
    } catch (std::exception const& e) {
        record_error_and_log("projection resolution failed", e.what());
        return false;
    }
    archive_reader->set_projection(projection);

    auto output_handler{
            create_output_handler(command_line_arguments, archive_reader->get_archive_id())
    };
    if (output_handler.has_error()) {
        record_error_and_log(
                "output handler creation failed",
                fmt::format(
                        "Failed to create output handler: {} - {}.",
                        output_handler.error().category().name(),
                        output_handler.error().message()
                )
        );
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
    auto const success{output.filter()};
    if (nullptr != telemetry_span) {
        if (false == success) {
            telemetry_span->set_error("archive filtering failed");
        }
        telemetry_span->set_termination_stage(output.get_termination_stage());
        telemetry_span->set_search_result_metrics(output.get_result_metrics());
    }
    return success;
}

auto handle_experimental_queries(CommandLineArguments const& cli_args) -> int {
    auto const& query{cli_args.get_query()};
    if (CommandLineArguments::cLogShapeStatsQuery != query
        && CommandLineArguments::cSchemaTreeStatsQuery != query)
    {
        return -1;
    }
    if (false == cli_args.experimental().has_value()) {
        throw std::invalid_argument(fmt::format("--experimental must be set to run {}", query));
    }
    auto archive_reader{std::make_shared<clp_s::ArchiveReader>()};
    for (auto const& input_path : cli_args.get_input_paths()) {
        try {
            archive_reader->open(
                    input_path,
                    clp_s::ArchiveReader::Options{
                            cli_args.get_network_auth(),
                            cli_args.experimental().has_value()
                    }
            );
        } catch (std::exception const& e) {
            SPDLOG_ERROR("Failed to open archive - {}", e.what());
            return 1;
        }
        auto output_handler{create_output_handler(cli_args, archive_reader->get_archive_id())};
        if (output_handler.has_error()) {
            SPDLOG_ERROR("Failed to create output handler - {}", output_handler.error().message());
            return 2;
        }
        auto const shape_stats{archive_reader->get_log_shape_stats()};
        if (CommandLineArguments::cLogShapeStatsQuery == query) {
            auto shape_dict{archive_reader->get_log_shape_dictionary()};
            shape_dict->read_entries();
            for (clpp::log_shape_id_t i{0}; i < shape_stats.size(); ++i) {
                nlohmann::json entry{
                        {"id", i},
                        {"count", shape_stats.at(i).get_count()},
                        {std::string{clpp::cShapeFunction},
                         std::string{shape_dict->get_entry(i).get_value()}}
                };
                output_handler.value()->write(entry.dump());
                output_handler.value()->write("\n");
            }
        } else if (CommandLineArguments::cSchemaTreeStatsQuery == query) {
            auto nodes{nlohmann::json::array()};
            for (auto const& node : archive_reader->get_schema_tree()->get_nodes()) {
                nodes.push_back({
                        {"id", node.get_id()},
                        {"parentId", node.get_parent_id()},
                        {"key", std::string{node.get_key_name()}},
                        {"type", static_cast<int>(node.get_type())},
                        {"count", node.get_count()},
                        {"children", node.get_children_ids()},
                });
            }
            output_handler.value()->write(nodes.dump());
            output_handler.value()->write("\n");
        }
        if (auto ec{output_handler.value()->flush()}; clp_s::ErrorCode::ErrorCodeSuccess != ec) {
            SPDLOG_ERROR("Failed to flush output handler. Error code: {}", std::to_string(ec));
            return 3;
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

    mongocxx::instance const mongocxx_instance{};
#if CLP_BUILD_CLP_S_ENABLE_CURL
    clp::CurlGlobalInstance const curl_instance{};
#endif

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

    std::optional<TelemetryContext> telemetry_context;
    if (command_line_arguments.get_enable_telemetry()) {
        telemetry_context.emplace();
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
        option.m_experimental = command_line_arguments.experimental().has_value();
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
        auto const& query{command_line_arguments.get_query()};
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

        int reducer_socket_fd{-1};
        if (std::holds_alternative<CommandLineArguments::ReducerOutputHandlerOptions>(
                    command_line_arguments.get_output_handler_options()
            ))
        {
            auto const& options{std::get<CommandLineArguments::ReducerOutputHandlerOptions>(
                    command_line_arguments.get_output_handler_options()
            )};
            reducer_socket_fd
                    = reducer::connect_to_reducer(options.host, options.port, options.job_id);
            if (-1 == reducer_socket_fd) {
                SPDLOG_ERROR("Failed to connect to reducer");
                return 1;
            }
        }

        auto archive_reader = std::make_shared<clp_s::ArchiveReader>();
        for (auto const& input_path : command_line_arguments.get_input_paths()) {
            if (std::string::npos != input_path.path.find(clp::ir::cIrFileExtension)) {
                auto const result{clp_s::search_kv_ir_stream(
                        input_path,
                        command_line_arguments,
                        expr->copy(),
                        reducer_socket_fd
                )};
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
                    || KvIrSearchError{KvIrSearchErrorEnum::AggregationSupportNotImplemented}
                               == error)
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

            std::shared_ptr<SearchTelemetrySpan> telemetry_span;
            if (command_line_arguments.get_enable_telemetry()) {
                telemetry_span = std::make_shared<SearchTelemetrySpan>();
            }
            try {
                archive_reader->open(
                        input_path,
                        clp_s::ArchiveReader::Options{
                                command_line_arguments.get_network_auth(),
                                command_line_arguments.experimental().has_value()
                        }
                );
            } catch (std::exception const& e) {
                SPDLOG_ERROR("Failed to open archive - {}", e.what());
                if (nullptr != telemetry_span) {
                    telemetry_span->set_error("failed to open archive");
                }
                return 1;
            }
            if (false
                == search_archive(
                        command_line_arguments,
                        archive_reader,
                        expr->copy(),
                        telemetry_span
                ))
            {
                return 1;
            }
            archive_reader->close();
        }
    }

    return 0;
}
