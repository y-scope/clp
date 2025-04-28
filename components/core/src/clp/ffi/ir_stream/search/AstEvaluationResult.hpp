#ifndef CLP_FFI_IR_STREAM_SEARCH_ASTEVALUATIONRESULT_HPP
#define CLP_FFI_IR_STREAM_SEARCH_ASTEVALUATIONRESULT_HPP

#include <cstdint>

namespace clp::ffi::ir_stream::search {
/**
 * Enum representing the result of evaluating a search AST.
 */
enum class AstEvaluationResult : uint8_t {
    True,
    False,

    // The AST evaluation is intentionally skipped because it belongs to a pruned branch of the
    // parent tree.
    Pruned,
};
}  // namespace clp::ffi::ir_stream::search

#endif  // CLP_FFI_IR_STREAM_SEARCH_ASTEVALUATIONRESULT_HPP
