#ifndef CLP_FFI_IR_STREAM_SEARCH_NEWPROJECTEDSCHEMATREENODECALLBACKREQ_HPP
#define CLP_FFI_IR_STREAM_SEARCH_NEWPROJECTEDSCHEMATREENODECALLBACKREQ_HPP

#include <cstddef>
#include <string_view>
#include <type_traits>
#include <utility>

#include <ystdlib/error_handling/Result.hpp>

#include "../../SchemaTree.hpp"

namespace clp::ffi::ir_stream::search {
/**
 * Requirements for a callback that's called whenever:
 *
 * 1. a new schema-tree node is deserialized from the IR stream; and
 * 2. that node corresponds to one of the projected key paths in the query.
 *
 * @tparam NewProjectedSchemaTreeNodeCallbackType
 * @param arg_0 Whether the schema-tree node is for an auto-generated kv-pair.
 * @param arg_1 The ID of the schema-tree node.
 * @param arg_2 The projected key path and its original index in the list of projections.
 * @return A void result on success or a user-defined error code indicating the failure.
 */
template <typename NewProjectedSchemaTreeNodeCallbackType>
concept NewProjectedSchemaTreeNodeCallbackReq = std::is_invocable_r_v<
        ystdlib::error_handling::Result<void>,
        NewProjectedSchemaTreeNodeCallbackType,
        bool,
        SchemaTree::Node::id_t,
        std::pair<std::string_view, size_t>
>;
}  // namespace clp::ffi::ir_stream::search

#endif  // CLP_FFI_IR_STREAM_SEARCH_NEWPROJECTEDSCHEMATREENODECALLBACKREQ_HPP
