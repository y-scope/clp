#ifndef CLP_FFI_IR_STREAM_SEARCH_ASTEVALUATIONRESULT_HPP
#define CLP_FFI_IR_STREAM_SEARCH_ASTEVALUATIONRESULT_HPP

#include <cstdint>
#include <type_traits>

namespace clp::ffi::ir_stream::search {
/**
 * Enum representing the result of evaluating a search AST.
 */
enum AstEvaluationResult : uint8_t {
    True = 1,
    False = 1 << 1,

    // The AST evaluation is intentionally skipped because it belongs to a pruned branch of the
    // parent tree.
    Pruned = 1 << 2,
};

using ast_evaluation_result_bitmask_t = std::underlying_type_t<AstEvaluationResult>;
}  // namespace clp::ffi::ir_stream::search

#endif  // CLP_FFI_IR_STREAM_SEARCH_ASTEVALUATIONRESULT_HPP
