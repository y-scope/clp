#ifndef CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLERREQ_HPP
#define CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLERREQ_HPP

#include <type_traits>

#include "NewProjectedSchemaTreeNodeCallbackReq.hpp"
#include "QueryHandler.hpp"

namespace clp::ffi::ir_stream::search {
/**
 * Defines an empty query handler that can be used with `clp::ffi::ir_stream::Deserializer` to
 * deserialize an IR stream without performing any query evaluation.
 */
struct EmptyQueryHandler {};

/**
 * A type trait to determine if a given type is an instantiation of
 * `clp::ffi::ir_stream::search::QueryHandler`.
 * @tparam T The type to check.
 */
template <typename T>
struct IsNonEmptyQueryHandler : std::false_type {};

/**
 * Specialization of `IsNonEmptyQueryHandler` for `clp::ffi::ir_stream::search::QueryHandler`.
 * @tparam NewProjectedSchemaTreeNodeCallbackType
 */
template <NewProjectedSchemaTreeNodeCallbackReq NewProjectedSchemaTreeNodeCallbackType>
struct IsNonEmptyQueryHandler<QueryHandler<NewProjectedSchemaTreeNodeCallbackType>>
        : std::true_type {};

/**
 * Requirements for a query handler that can be used with `clp::ffi::ir_stream::Deserializer`. A
 * valid query handler must be:
 *
 * - an `EmptyQueryHandler` or satisfy the `IsNonEmptyQueryHandler` trait.
 * - move constructible.
 *
 * @tparam QueryHandlerType The type to check.
 */
template <typename QueryHandlerType>
concept QueryHandlerReq = (std::is_same_v<QueryHandlerType, EmptyQueryHandler>
                           || IsNonEmptyQueryHandler<QueryHandlerType>::value)
                          && std::is_move_constructible_v<QueryHandlerType>;
}  // namespace clp::ffi::ir_stream::search

#endif  // CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLERREQ_HPP
