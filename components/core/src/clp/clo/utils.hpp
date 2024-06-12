#ifndef CLP_CLO_UTILS_HPP
#define CLP_CLO_UTILS_HPP

#include "ErrorCode.hpp"
#include "TraceableException.hpp"

namespace clp::clo {
// Types
class CloOperationFailed : public TraceableException {
public:
    // Constructors
    CloOperationFailed(ErrorCode error_code, char const* const filename, int line_number)
            : TraceableException(error_code, filename, line_number) {}

    // Methods
    [[nodiscard]] char const* what() const noexcept override { return "CLO operation failed"; }
};
}  // namespace clp::clo
#endif  // CLP_CLO_UTILS_HPP
