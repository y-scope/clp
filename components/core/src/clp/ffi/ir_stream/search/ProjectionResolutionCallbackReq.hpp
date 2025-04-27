#ifndef CLP_FFI_IR_STREAM_SEARCH_PROJECTIONRESOLUTIONCALLBACKREQ_HPP
#define CLP_FFI_IR_STREAM_SEARCH_PROJECTIONRESOLUTIONCALLBACKREQ_HPP

#include <string_view>
#include <type_traits>

#include <outcome/single-header/outcome.hpp>

#include "../../SchemaTree.hpp"

namespace clp::ffi::ir_stream::search {
/**
 * Requirements for a callback that's called whenever:
 *
 * 1. a new schema-tree node is read from the IR stream; and
 * 2. that node corresponds to one of the projected key paths in the query.
 *
 * @tparam ProjectionResolutionCallbackType
 * @param arg_0 Whether the schema-tree node is for an auto-generated kv-pair.
 * @param arg_1 The ID of the schema-tree node.
 * @param arg_2 The projected key path.
 * @return A void result on success or a user-defined error code indicating the failure.
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
