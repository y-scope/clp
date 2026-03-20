#include "ErrorCode.hpp"

#include <string>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace {
using clp_s::filter::ErrorCodeEnum;
using ErrorCategory = ystdlib::error_handling::ErrorCategory<ErrorCodeEnum>;
}  // namespace

template <>
auto ErrorCategory::name() const noexcept -> char const* {
    return "clp_s::filter::ErrorCode";
}

template <>
auto ErrorCategory::message(ErrorCodeEnum error_enum) const -> std::string {
    switch (error_enum) {
        case ErrorCodeEnum::InvalidFalsePositiveRate:
            return "false-positive rate must be in the range [1e-6, 1)";
        case ErrorCodeEnum::ParameterComputationOutOfRange:
            return "bloom filter parameter computation overflowed or produced invalid values";
        case ErrorCodeEnum::UnsupportedHashAlgorithm:
            return "bloom filter hash algorithm is unsupported";
        case ErrorCodeEnum::CorruptFilterPayload:
            return "bloom filter payload is malformed or inconsistent";
        case ErrorCodeEnum::ReadFailure:
            return "failed to read filter data from reader";
        case ErrorCodeEnum::UnsupportedFilterType:
            return "filter type is unsupported";
        case ErrorCodeEnum::UnsupportedFilterNormalization:
            return "filter normalization is unsupported";
        case ErrorCodeEnum::CorruptFilterFile:
            return "filter file is malformed or inconsistent";
    }
    return "unknown error code enum";
}
