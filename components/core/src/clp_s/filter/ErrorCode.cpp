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
            return "False-positive rate must be in the range [1e-6, 1).";
        case ErrorCodeEnum::ParameterComputationOutOfRange:
            return "Bloom filter parameter computation overflowed or produced invalid values.";
        case ErrorCodeEnum::CorruptFilterPayload:
            return "Bloom filter payload is malformed or inconsistent.";
        case ErrorCodeEnum::ReadFailure:
            return "Failed to read Bloom filter payload from reader.";
    }
    return "Unknown error code enum";
}
