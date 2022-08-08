#ifndef NETWORKING_SOCKETOPERATIONFAILED_HPP
#define NETWORKING_SOCKETOPERATIONFAILED_HPP

// Project headers
#include "../ErrorCode.hpp"
#include "../TraceableException.hpp"

namespace networking {
    class SocketOperationFailed : public TraceableException {
    public:
        // Constructors
        SocketOperationFailed (ErrorCode error_code, const char* const filename, int line_number) :
                TraceableException(error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "Socket operation failed";
        }
    };
}

#endif //NETWORKING_SOCKETOPERATIONFAILED_HPP
