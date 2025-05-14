#include "kv_ir_search.hpp"

#include <sys/errno.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <outcome/outcome.hpp>
#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/ErrorCode.hpp>

#include "../clp/ErrorCode.hpp"
#include "../clp/ffi/ir_stream/decoding_methods.hpp"
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
#include "search/ast/Expression.hpp"

using clp_s::KvIrSearchError;
using clp_s::KvIrSearchErrorEnum;
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
            -> outcome_v2::std_result<IrUnitHandler>;

    // Delete copy constructor and assignment operator
    IrUnitHandler(IrUnitHandler const&) = delete;
    auto operator=(IrUnitHandler const&) -> IrUnitHandler& = delete;

    // Default move constructor and assignment operator
    IrUnitHandler(IrUnitHandler&&) = default;
    auto operator=(IrUnitHandler&&) -> IrUnitHandler& = default;

    // Destructor
    ~IrUnitHandler() = default;

    // Methods implementing `IrUnitHandlerInterface`
    [[nodiscard]] auto handle_log_event(KeyValuePairLogEvent&& log_event) -> IRErrorCode;

    [[nodiscard]] static auto handle_utc_offset_change(
            [[maybe_unused]] UtcOffset utc_offset_old,
            [[maybe_unused]] UtcOffset utc_offset_new
    ) -> IRErrorCode {
        return IRErrorCode::IRErrorCode_Decode_Error;
    }

    [[nodiscard]] static auto handle_schema_tree_node_insertion(
            [[maybe_unused]] bool is_auto_generated,
            [[maybe_unused]] clp::ffi::SchemaTree::NodeLocator schema_tree_node_locator,
            [[maybe_unused]] std::shared_ptr<clp::ffi::SchemaTree const> const& schema_tree
    ) -> IRErrorCode {
        return IRErrorCode::IRErrorCode_Success;
    }

    [[nodiscard]] auto handle_end_of_stream() -> IRErrorCode {
        m_is_complete = true;
        return IRErrorCode::IRErrorCode_Success;
    }

private:
    // Constructor
    IrUnitHandler() = default;

    // Variable
    bool m_is_complete{false};
};

/**
 * Deserializes the kv-pair IR stream from the given stream reader and performs query search.
 * @param stream_reader The stream reader to read the kv-pair IR stream from.
 * @param command_line_arguments
 * @param query
 * @param reducer_socket_fd
 * @return A void result on success, or an error code indicating the failure:
 * - Forwards `clp::ffi::ir_stream::Deserializer::create`'s return values.
 * - Forwards `clp::ffi::ir_stream::Deserializer::deserialize_next_ir_unit`'s return values.
 * - Forwards `clp::ffi::ir_stream::search::QueryHandler::create`'s return values.
 * - Forwards `IrUnitHandler::create`'s return values.
 */
[[nodiscard]] auto deserialize_and_search_kv_ir_stream(
        clp::ReaderInterface& stream_reader,
        CommandLineArguments const& command_line_arguments,
        std::shared_ptr<search::ast::Expression> query,
        int reducer_socket_fd
) -> outcome_v2::std_result<void>;

auto IrUnitHandler::create(
        CommandLineArguments const& command_line_arguments,
        [[maybe_unused]] int reducer_socket_fd
) -> outcome_v2::std_result<IrUnitHandler> {
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

auto IrUnitHandler::handle_log_event(clp::ffi::KeyValuePairLogEvent&& log_event) -> IRErrorCode {
    return IRErrorCode::IRErrorCode_Success;
}

auto deserialize_and_search_kv_ir_stream(
        clp::ReaderInterface& stream_reader,
        CommandLineArguments const& command_line_arguments,
        std::shared_ptr<search::ast::Expression> query,
        int reducer_socket_fd
) -> outcome_v2::std_result<void> {
    auto trivial_new_projected_schema_tree_node_callback
            = [](
                      [[maybe_unused]] bool is_auto_generated,
                      [[maybe_unused]] SchemaTree::Node::id_t node_id,
                      [[maybe_unused]] std::string_view projected_key_path
              ) -> outcome_v2::std_result<void> { return outcome_v2::success(); };
    using QueryHandlerType = clp::ffi::ir_stream::search::QueryHandler<
            decltype(trivial_new_projected_schema_tree_node_callback)>;

    auto deserializer{OUTCOME_TRYX(make_deserializer(
            stream_reader,
            OUTCOME_TRYX(IrUnitHandler::create(command_line_arguments, reducer_socket_fd)),
            OUTCOME_TRYX(
                    QueryHandlerType::create(
                            trivial_new_projected_schema_tree_node_callback,
                            std::move(query),
                            {},
                            false
                    )
            )
    ))};

    while (IrUnitType::EndOfStream
           != OUTCOME_TRYX(deserializer.deserialize_next_ir_unit(stream_reader)))
    {}

    return outcome_v2::success();
}
}  // namespace

[[nodiscard]] auto search_kv_ir_stream(
        std::string const& stream_path,
        CommandLineArguments const& command_line_arguments,
        std::shared_ptr<search::ast::Expression> query,
        int reducer_socket_fd
) -> outcome_v2::std_result<void> {
    clp::streaming_compression::zstd::Decompressor decompressor;
    if (auto const err{decompressor.open(stream_path)}; clp::ErrorCode_Success != err) {
        if (clp::ErrorCode_FileNotFound == err) {
            SPDLOG_ERROR("kv-ir stream '{}' does not exist.", stream_path);
        } else if (clp::ErrorCode_errno == err) {
            SPDLOG_ERROR("Failed to open kv-ir stream '{}', errno={}", stream_path, errno);
        } else {
            SPDLOG_ERROR("Failed to open kv-ir stream '{}', error_code={}", stream_path, err);
        }
        return KvIrSearchError{KvIrSearchErrorEnum::ClpLegacyError};
    }

    if (false == command_line_arguments.get_projection_columns().empty()) {
        SPDLOG_ERROR("Projection support on kv-ir stream search is not implemented.");
        return KvIrSearchError{KvIrSearchErrorEnum::ProjectionSupportNotImplemented};
    }

    try {
        OUTCOME_TRYV(deserialize_and_search_kv_ir_stream(
                decompressor,
                command_line_arguments,
                std::move(query),
                reducer_socket_fd
        ));
    } catch (clp::TraceableException const& ex) {
        // TODO
    }

    decompressor.close();
    return outcome_v2::success();
}
}  // namespace clp_s

template <>
auto KvIrSearchErrorCategory::name() const noexcept -> char const* {
    return "clp_s::kv_ir_search";
}

template <>
auto KvIrSearchErrorCategory::message(clp_s::KvIrSearchErrorEnum error_enum) const -> std::string {
    switch (error_enum) {
        case KvIrSearchErrorEnum::ClpLegacyError:
            return "clp legacy error.";
        case KvIrSearchErrorEnum::ProjectionSupportNotImplemented:
            return "Projection support is not implemented yet.";
        case KvIrSearchErrorEnum::UnsupportedOutputHandlerType:
            return "Unsupported output handler type.";
        default:
            return "Unknown error.";
    }
}
