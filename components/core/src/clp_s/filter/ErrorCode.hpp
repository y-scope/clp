#ifndef CLP_S_FILTER_ERROR_CODE_HPP
#define CLP_S_FILTER_ERROR_CODE_HPP

#include <cstdint>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace clp_s::filter {
enum class ErrorCodeEnum : uint8_t {
    InvalidFalsePositiveRate = 1,
    ParameterComputationOutOfRange,
    UnsupportedHashAlgorithm,
    CorruptFilterPayload,
    ReadFailure,
    UnsupportedFilterType,
    UnsupportedFilterNormalization,
    CorruptFilterFile,
};

using ErrorCode = ystdlib::error_handling::ErrorCode<ErrorCodeEnum>;
}  // namespace clp_s::filter

YSTDLIB_ERROR_HANDLING_MARK_AS_ERROR_CODE_ENUM(clp_s::filter::ErrorCodeEnum);

#endif  // CLP_S_FILTER_ERROR_CODE_HPP
