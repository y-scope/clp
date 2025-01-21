#ifndef CLP_FFI_IR_STREAM_IRUNITHANDLERINTERFACE_HPP
#define CLP_FFI_IR_STREAM_IRUNITHANDLERINTERFACE_HPP

#include <concepts>
#include <utility>

#include "../../time_types.hpp"
#include "../KeyValuePairLogEvent.hpp"
#include "../SchemaTree.hpp"
#include "decoding_methods.hpp"

namespace clp::ffi::ir_stream {
/**
 * Concept that defines the IR unit handler interface.
 */
template <typename Handler>
concept IrUnitHandlerInterface = requires(
        Handler handler,
        KeyValuePairLogEvent&& log_event,
        bool is_auto_generated,
        UtcOffset utc_offset_old,
        UtcOffset utc_offset_new,
        SchemaTree::NodeLocator schema_tree_node_locator
) {
    /**
     * Handles a log event IR unit.
     * @param log_event The deserialized result from IR deserializer.
     * @return IRErrorCode::Success on success, user-defined error code on failures.
     */
    {
        handler.handle_log_event(std::forward<KeyValuePairLogEvent&&>(log_event))
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
     * @return IRErrorCode::Success on success, user-defined error code on failures.
     */
    {
        handler.handle_schema_tree_node_insertion(is_auto_generated, schema_tree_node_locator)
    } -> std::same_as<IRErrorCode>;

    /**
     * Handles an end-of-stream indicator IR unit.
     * @return IRErrorCode::Success on success, user-defined error code on failures.
     */
    {
        handler.handle_end_of_stream()
    } -> std::same_as<IRErrorCode>;
};
}  // namespace clp::ffi::ir_stream

#endif  // CLP_FFI_IR_STREAM_IRUNITHANDLERINTERFACE_HPP
