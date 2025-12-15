#include "kv_ir_search.hpp"

#include <cerrno>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <fmt/format.h>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/ErrorCode.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include "../clp/ErrorCode.hpp"
#include "../clp/ffi/ir_stream/Deserializer.hpp"
#include "../clp/ffi/ir_stream/IrUnitType.hpp"
#include "../clp/ffi/ir_stream/search/QueryHandler.hpp"
#include "../clp/ffi/KeyValuePairLogEvent.hpp"
#include "../clp/ffi/SchemaTree.hpp"
#include "../clp/ReaderInterface.hpp"
#include "../clp/spdlog_with_specializations.hpp"
#include "../clp/streaming_compression/zstd/Decompressor.hpp"
#include "../clp/time_types.hpp"
#include "../clp/TraceableException.hpp"
#include "CommandLineArguments.hpp"
#include "InputConfig.hpp"
#include "search/ast/Expression.hpp"
#include "search/ast/SetTimestampLiteralPrecision.hpp"
#include "search/ast/TimestampLiteral.hpp"

// This include has a circular dependency with the `.inc` file.
// The following clang-tidy suppression should be removed once the circular dependency is resolved.
// NOLINTNEXTLINE(misc-header-include-cycle)
#include "../clp/ffi/ir_stream/decoding_methods.hpp"

using clp_s::KvIrSearchError;
using clp_s::KvIrSearchErrorEnum;
using clp_s::search::ast::SetTimestampLiteralPrecision;
using clp_s::search::ast::TimestampLiteral;
using KvIrSearchErrorCategory = ystdlib::error_handling::ErrorCategory<KvIrSearchErrorEnum>;

namespace clp_s {
namespace {
using clp::ffi::ir_stream::IRErrorCode;
using clp::ffi::ir_stream::IrUnitType;
using clp::ffi::ir_stream::make_deserializer;
using clp::ffi::KeyValuePairLogEvent;
using clp::ffi::SchemaTree;
using clp::UtcOffset;

class IrUnitHandler {
public:
    // Factory function
    /**
     * @param command_line_arguments
     * @param reducer_socket_fd
     * @return A result containing the created IrUnitHandler on success, or an error code indicating
     * the failure:
     * - KvIrSearchErrorEnum::UnsupportedOutputHandlerType if the output handler type is not
     *   supported.
     */
    [[nodiscard]] static auto
    create(CommandLineArguments const& command_line_arguments, int reducer_socket_fd)
            -> ystdlib::error_handling::Result<IrUnitHandler>;

    // Delete copy constructor and assignment operator
    IrUnitHandler(IrUnitHandler const&) = delete;
    auto operator=(IrUnitHandler const&) -> IrUnitHandler& = delete;

    // Default move constructor and assignment operator
    IrUnitHandler(IrUnitHandler&&) = default;
    auto operator=(IrUnitHandler&&) -> IrUnitHandler& = default;

    // Destructor
    ~IrUnitHandler() = default;

    // Methods implementing `IrUnitHandlerInterface`
    [[nodiscard]] auto
    handle_log_event(KeyValuePairLogEvent log_event, [[maybe_unused]] size_t log_event_idx)
            -> IRErrorCode;

    [[nodiscard]] static auto handle_utc_offset_change(
            [[maybe_unused]] UtcOffset utc_offset_old,
            [[maybe_unused]] UtcOffset utc_offset_new
    ) -> IRErrorCode {
        return IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] static auto handle_schema_tree_node_insertion(
            [[maybe_unused]] bool is_auto_generated,
            [[maybe_unused]] clp::ffi::SchemaTree::NodeLocator schema_tree_node_locator,
            [[maybe_unused]] std::shared_ptr<clp::ffi::SchemaTree const> const& schema_tree
    ) -> IRErrorCode {
        return IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] auto handle_end_of_stream() -> IRErrorCode {
        return IRErrorCode::IRErrorCode_Success;
    }

private:
    // Constructor
    IrUnitHandler() = default;
};

/**
 * Deserializes the kv-pair IR stream from the given stream reader and performs query search.
 * @param stream_reader The stream reader to read the kv-pair IR stream from.
 * @param command_line_arguments
 * @param query
 * @param reducer_socket_fd
 * @return A void result on success, or an error code indicating the failure:
 * - KvIrSearchErrorEnum::DeserializerCreationFailure if `clp::ffi::ir_stream::Deserializer::create`
 *   failed. This specific error code is returned instead of propagating the return values of
 *   `clp::ffi::ir_stream::Deserializer::create`, allowing callers to identify cases where the input
 *   might not be a kv-pair IR stream.
 * - Forwards `clp::ffi::ir_stream::Deserializer::deserialize_next_ir_unit`'s return values.
 * - Forwards `clp::ffi::ir_stream::search::QueryHandler::create`'s return values.
 * - Forwards `IrUnitHandler::create`'s return values.
 */
[[nodiscard]] auto deserialize_and_search_kv_ir_stream(
        clp::ReaderInterface& stream_reader,
        CommandLineArguments const& command_line_arguments,
        std::shared_ptr<search::ast::Expression> query,
        int reducer_socket_fd
) -> ystdlib::error_handling::Result<void>;

auto IrUnitHandler::create(
        CommandLineArguments const& command_line_arguments,
        [[maybe_unused]] int reducer_socket_fd
) -> ystdlib::error_handling::Result<IrUnitHandler> {
    switch (command_line_arguments.get_output_handler_type()) {
        case CommandLineArguments::OutputHandlerType::Stdout:
            break;
        case CommandLineArguments::OutputHandlerType::Network:
        case CommandLineArguments::OutputHandlerType::Reducer:
        case CommandLineArguments::OutputHandlerType::ResultsCache:
            SPDLOG_ERROR(
                    "kv-ir search: Only stdout output is supported in the current implementation."
            );
            return KvIrSearchError{KvIrSearchErrorEnum::UnsupportedOutputHandlerType};
        default:
            SPDLOG_ERROR("kv-ir search: Unknown output method.");
            return KvIrSearchError{KvIrSearchErrorEnum::UnsupportedOutputHandlerType};
    }
    return IrUnitHandler{};
}

/**
 * The following clang-tidy check is disabled because this function is intentionally designed as a
 * non-static member function for the following reasons:
 * - It is expected to be extended in the future to support additional search features.
 * - It must be invoked through an object created by the factory function, which ensures all
 *   necessary validations are performed.
 */
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto IrUnitHandler::handle_log_event(
        clp::ffi::KeyValuePairLogEvent log_event,
        [[maybe_unused]] size_t log_event_idx
) -> IRErrorCode {
    auto const serialize_result{log_event.serialize_to_json()};
    if (serialize_result.has_error()) {
        SPDLOG_ERROR(
                "kv-ir search: Failed to serialize kv-pair log event to JSON objects."
                " error_category={}, error={}",
                serialize_result.error().category().name(),
                serialize_result.error().message()
        );
        return IRErrorCode::IRErrorCode_Decode_Error;
    }
    auto const& [auto_gen_kv_pairs, user_gen_kv_pairs] = serialize_result.value();

    try {
        constexpr std::string_view cAutoGenKey{"\"auto_generated_kv_pairs\""};
        constexpr std::string_view cUserGenKey{"\"user_generated_kv_pairs\""};
        std::cout << fmt::format(
                "{{{}:{},{}:{}}}\n",
                cAutoGenKey,
                auto_gen_kv_pairs.dump(),
                cUserGenKey,
                user_gen_kv_pairs.dump()
        );
    } catch (nlohmann::json::exception const& ex) {
        SPDLOG_ERROR(
                "kv-ir search: Failed to serialize kv-pair log event into JSON strings."
                " ErrorMessage={}",
                ex.what()
        );
        return IRErrorCode::IRErrorCode_Corrupted_IR;
    }

    return IRErrorCode::IRErrorCode_Success;
}

auto deserialize_and_search_kv_ir_stream(
        clp::ReaderInterface& stream_reader,
        CommandLineArguments const& command_line_arguments,
        std::shared_ptr<search::ast::Expression> query,
        int reducer_socket_fd
) -> ystdlib::error_handling::Result<void> {
    auto trivial_new_projected_schema_tree_node_callback
            = []([[maybe_unused]] bool is_auto_generated,
                 [[maybe_unused]] SchemaTree::Node::id_t node_id,
                 [[maybe_unused]] std::pair<std::string_view, size_t> projected_key_and_index)
            -> ystdlib::error_handling::Result<void> { return ystdlib::error_handling::success(); };
    using QueryHandlerType = clp::ffi::ir_stream::search::
            QueryHandler<decltype(trivial_new_projected_schema_tree_node_callback)>;

    auto ir_unit_handler{YSTDLIB_ERROR_HANDLING_TRYX(
            IrUnitHandler::create(command_line_arguments, reducer_socket_fd)
    )};
    auto query_handler{YSTDLIB_ERROR_HANDLING_TRYX(
            QueryHandlerType::create(
                    trivial_new_projected_schema_tree_node_callback,
                    std::move(query),
                    {},
                    false == command_line_arguments.get_ignore_case()
            )
    )};

    auto deserializer_result{
            make_deserializer(stream_reader, std::move(ir_unit_handler), std::move(query_handler))
    };

    if (deserializer_result.has_error()) {
        return KvIrSearchError{KvIrSearchErrorEnum::DeserializerCreationFailure};
    }

    auto& deserializer{deserializer_result.value()};
    while (IrUnitType::EndOfStream
           != YSTDLIB_ERROR_HANDLING_TRYX(deserializer.deserialize_next_ir_unit(stream_reader)))
    {}

    return ystdlib::error_handling::success();
}
}  // namespace

auto search_kv_ir_stream(
        Path const& stream_path,
        CommandLineArguments const& command_line_arguments,
        std::shared_ptr<search::ast::Expression> query,
        int reducer_socket_fd
) -> ystdlib::error_handling::Result<void> {
    if (false == command_line_arguments.get_projection_columns().empty()) {
        SPDLOG_ERROR("kv-ir search: Projection support is not implemented.");
        return KvIrSearchError{KvIrSearchErrorEnum::ProjectionSupportNotImplemented};
    }

    if (command_line_arguments.do_count_by_time_aggregation()
        || command_line_arguments.do_count_results_aggregation())
    {
        SPDLOG_ERROR("kv-ir search: Count support is not implemented.");
        return KvIrSearchError{KvIrSearchErrorEnum::CountSupportNotImplemented};
    }

    auto const raw_reader{
            try_create_reader(stream_path, command_line_arguments.get_network_auth())
    };
    if (nullptr == raw_reader) {
        return KvIrSearchError{KvIrSearchErrorEnum::StreamReaderCreationFailure};
    }

    if (command_line_arguments.get_search_begin_ts().has_value()
        || command_line_arguments.get_search_end_ts().has_value())
    {
        SPDLOG_WARN(
                "kv-ir search: Timestamp filters are currently not supported."
                " Values will be ignored."
        );
    }

    SetTimestampLiteralPrecision date_precision_pass{TimestampLiteral::Precision::Milliseconds};
    query = date_precision_pass.run(query);

    try {
        clp::streaming_compression::zstd::Decompressor decompressor;
        constexpr size_t cReaderBufferSize{64L * 1024L};  // 64 KB
        decompressor.open(*raw_reader, cReaderBufferSize);
        YSTDLIB_ERROR_HANDLING_TRYV(deserialize_and_search_kv_ir_stream(
                decompressor,
                command_line_arguments,
                std::move(query),
                reducer_socket_fd
        ));
        decompressor.close();
    } catch (clp::TraceableException const& ex) {
        auto const err{ex.get_error_code()};
        if (clp::ErrorCode_errno == err) {
            SPDLOG_ERROR(
                    "kv-ir search failed on `clp::TraceableException`: errno={}, msg={}",
                    errno,
                    ex.what()
            );
        } else {
            SPDLOG_ERROR(
                    "kv-ir search failed on `clp::TraceableException`: error_code={}, msg={}",
                    err,
                    ex.what()
            );
        }
        return KvIrSearchError{KvIrSearchErrorEnum::ClpLegacyError};
    }

    return ystdlib::error_handling::success();
}
}  // namespace clp_s

template <>
auto KvIrSearchErrorCategory::name() const noexcept -> char const* {
    return "clp_s::kv_ir_search";
}

template <>
auto KvIrSearchErrorCategory::message(KvIrSearchErrorEnum error_enum) const -> std::string {
    switch (error_enum) {
        case KvIrSearchErrorEnum::ClpLegacyError:
            return "clp legacy error.";
        case KvIrSearchErrorEnum::CountSupportNotImplemented:
            return "Count support is not implemented.";
        case KvIrSearchErrorEnum::DeserializerCreationFailure:
            return "Failed to create `clp::ffi::ir_stream::Deserializer`.";
        case KvIrSearchErrorEnum::ProjectionSupportNotImplemented:
            return "Projection support is not implemented.";
        case KvIrSearchErrorEnum::StreamReaderCreationFailure:
            return "Failed to create stream reader.";
        case KvIrSearchErrorEnum::UnsupportedOutputHandlerType:
            return "Unsupported output handler type.";
        default:
            return "Unknown error.";
    }
}
