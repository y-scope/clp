#ifndef CLP_S_KV_IR_SEARCH_HPP
#define CLP_S_KV_IR_SEARCH_HPP

#include <cstdint>
#include <memory>

#include <ystdlib/error_handling/ErrorCode.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include "CommandLineArguments.hpp"
#include "InputConfig.hpp"
#include "search/ast/Expression.hpp"

namespace clp_s {
enum class KvIrSearchErrorEnum : uint8_t {
    ClpLegacyError = 1,
    CountSupportNotImplemented,
    DeserializerCreationFailure,
    ProjectionSupportNotImplemented,
    StreamReaderCreationFailure,
    UnsupportedOutputHandlerType,
};

using KvIrSearchError = ystdlib::error_handling::ErrorCode<KvIrSearchErrorEnum>;

/**
 * Searches the given kv-pair IR stream with the given query.
 * @param stream_path The path to the kv-pair IR stream.
 * @param command_line_arguments
 * @param query
 * @return A void result on success, or an error code indicating the failure:
 * - KvIrSearchErrorEnum::ClpLegacyError if a `clp::TraceableException` is caught.
 * - KvIrSearchErrorEnum::CountSupportNotImplemented if count-related features are enabled.
 * - KvIrSearchErrorEnum::ProjectionSupportNotImplemented if projection is non-empty.
 * - KvIrSearchErrorEnum::StreamReaderCreationFailure if the stream reader cannot be successfully
 *   created.
 * - Forwards `deserialize_and_search_kv_ir_stream`'s return values.
 */
[[nodiscard]] auto search_kv_ir_stream(
        Path const& stream_path,
        CommandLineArguments const& command_line_arguments,
        std::shared_ptr<search::ast::Expression> query
) -> ystdlib::error_handling::Result<void>;
}  // namespace clp_s

YSTDLIB_ERROR_HANDLING_MARK_AS_ERROR_CODE_ENUM(clp_s::KvIrSearchErrorEnum);

#endif  // CLP_S_KV_IR_SEARCH_HPP
