#ifndef FFI_SEARCH_QUERYMETHODFAILED_HPP
#define FFI_SEARCH_QUERYMETHODFAILED_HPP

#include <string>

#include "../../TraceableException.hpp"

namespace ffi::search {
class QueryMethodFailed : public TraceableException {
public:
    // Constructors
    QueryMethodFailed(
            ErrorCode error_code,
            char const* const filename,
            int line_number,
            std::string message
    )
            : TraceableException(error_code, filename, line_number),
              m_message(std::move(message)) {}

    // Methods
    [[nodiscard]] char const* what() const noexcept override { return m_message.c_str(); }

private:
    std::string m_message;
};
}  // namespace ffi::search

#endif  // FFI_SEARCH_QUERYMETHODFAILED_HPP
