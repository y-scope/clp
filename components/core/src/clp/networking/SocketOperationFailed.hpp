#ifndef CLP_NETWORKING_SOCKETOPERATIONFAILED_HPP
#define CLP_NETWORKING_SOCKETOPERATIONFAILED_HPP

#include "../ErrorCode.hpp"
#include "../TraceableException.hpp"

namespace clp::networking {
class SocketOperationFailed : public TraceableException {
public:
    // Constructors
    SocketOperationFailed(ErrorCode error_code, char const* const filename, int line_number)
            : TraceableException(error_code, filename, line_number) {}

    // Methods
    [[nodiscard]] auto what() const noexcept -> char const* override {
        return "Socket operation failed";
    }
};
}  // namespace clp::networking

#endif  // CLP_NETWORKING_SOCKETOPERATIONFAILED_HPP
