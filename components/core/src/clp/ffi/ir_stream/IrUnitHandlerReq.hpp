#ifndef CLP_FFI_IR_STREAM_IRUNITHANDLERREQ_HPP
#define CLP_FFI_IR_STREAM_IRUNITHANDLERREQ_HPP

#include <concepts>
#include <memory>
#include <utility>

#include "../../time_types.hpp"
#include "../KeyValuePairLogEvent.hpp"
#include "../SchemaTree.hpp"

// This include has a circular dependency with the `.inc` file.
// The following clang-tidy suppression should be removed once the circular dependency is resolved.
// NOLINTNEXTLINE(misc-header-include-cycle)
#include "decoding_methods.hpp"

namespace clp::ffi::ir_stream {
/**
 * Requirement for the IR unit handler interface.
 * @tparam IrUnitHandlerType The type of the IR unit handler.
 */
template <typename IrUnitHandlerType>
concept IrUnitHandlerInterfaceReq = requires(
        IrUnitHandlerType handler,
        KeyValuePairLogEvent&& log_event,
        bool is_auto_generated,
        UtcOffset utc_offset_old,
        UtcOffset utc_offset_new,
        SchemaTree::NodeLocator schema_tree_node_locator,
        std::shared_ptr<SchemaTree const> schema_tree
) {
    /**
     * Handles a log event IR unit.
     * @param log_event The deserialized result from IR deserializer.
     * @return IRErrorCode::Success on success, user-defined error code on failures.
     */
    {
        handler.handle_log_event(std::forward<KeyValuePairLogEvent &&>(log_event))
    } -> std::same_as<IRErrorCode>;

    /**
     * Handles a UTC offset change IR unit.
     * @param utc_offset_old The offset before the change.
     * @param utc_offset_new The deserialized new offset.
     * @return IRErrorCode::Success on success, user-defined error code on failures.
     */
    {
        handler.handle_utc_offset_change(utc_offset_old, utc_offset_new)
    } -> std::same_as<IRErrorCode>;

    /**
     * Handles a schema tree node insertion IR unit.
     * @param is_auto_generated Whether the node is from the auto-gen keys schema tree.
     * @param schema_tree_node_locator The locator of the node to insert.
     * @param schema_tree The schema tree that contains the inserted node.
     * @return IRErrorCode::Success on success, user-defined error code on failures.
     */
    {
        handler.handle_schema_tree_node_insertion(
                is_auto_generated,
                schema_tree_node_locator,
                schema_tree
        )
    } -> std::same_as<IRErrorCode>;

    /**
     * Handles an end-of-stream indicator IR unit.
     * @return IRErrorCode::Success on success, user-defined error code on failures.
     */
    {
        handler.handle_end_of_stream()
    } -> std::same_as<IRErrorCode>;
};

/**
 * Requirement for an IR unit handler.
 * @tparam IrUnitHandlerType The type of the IR unit handler.
 */
template <typename IrUnitHandlerType>
concept IrUnitHandlerReq = std::move_constructible<IrUnitHandlerType>
                           && IrUnitHandlerInterfaceReq<IrUnitHandlerType>;
}  // namespace clp::ffi::ir_stream

#endif  // CLP_FFI_IR_STREAM_IRUNITHANDLERREQ_HPP
