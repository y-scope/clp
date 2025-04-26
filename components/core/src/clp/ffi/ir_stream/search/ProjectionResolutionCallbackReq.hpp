#ifndef CLP_FFI_IR_STREAM_SEARCH_PROJECTIONRESOLUTIONCALLBACKREQ_HPP
#define CLP_FFI_IR_STREAM_SEARCH_PROJECTIONRESOLUTIONCALLBACKREQ_HPP

#include <string_view>
#include <type_traits>

#include <outcome/single-header/outcome.hpp>

#include "../../SchemaTree.hpp"

namespace clp::ffi::ir_stream::search {
/**
 * Concept that defines the callback function signature for projection resolution.
 * @tparam ProjectionResolutionCallbackType
 *
 * Callback parameters:
 * - `bool is_auto_generated`: Whether the schema tree node is auto-generated.
 * - `SchemaTree::Node::id_t schema_tree_node_id`: The ID of the resolved schema tree node.
 * - `std::string_view key_path`: The key path of the resolved projection.
 *
 * Callback return values:
 * - A void result on success.
 * - A user-defined error code indicating the failure.
 */
template <typename ProjectionResolutionCallbackType>
concept ProjectionResolutionCallbackReq = std::is_invocable_r_v<
        outcome_v2::std_result<void>,
        ProjectionResolutionCallbackType,
        bool,
        SchemaTree::Node::id_t,
        std::string_view>;
}  // namespace clp::ffi::ir_stream::search

#endif  // CLP_FFI_IR_STREAM_SEARCH_PROJECTIONRESOLUTIONCALLBACKREQ_HPP
