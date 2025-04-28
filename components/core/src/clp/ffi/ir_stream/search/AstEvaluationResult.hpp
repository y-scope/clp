#ifndef CLP_FFI_IR_STREAM_SEARCH_ASTEVALUATIONRESULT_HPP
#define CLP_FFI_IR_STREAM_SEARCH_ASTEVALUATIONRESULT_HPP

#include <cstdint>

namespace clp::ffi::ir_stream::search {
/**
 * Enum representing the result of evaluating a search AST.
 *
 * Possible values:
 * - True:   The AST evaluates to `true`.
 * - False:  The AST evaluates to `false`.
 * - Pruned: The AST evaluation is intentionally skipped because it belongs to a pruned branch of
 *           the parent tree.
 */
enum class AstEvaluationResult : uint8_t {
    True,
    False,
    Pruned,
};
}  // namespace clp::ffi::ir_stream::search

#endif  // CLP_FFI_IR_STREAM_SEARCH_ASTEVALUATIONRESULT_HPP
