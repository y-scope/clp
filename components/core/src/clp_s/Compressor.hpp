// Code from CLP

#ifndef CLP_S_COMPRESSOR_HPP
#define CLP_S_COMPRESSOR_HPP

#include <cstdint>
#include <string>

#include <zstd.h>

#include "TraceableException.hpp"

namespace clp_s {
class Compressor {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    enum class CompressorType : uint8_t {
        ZSTD = 0x10,
        Passthrough = 0xFF,
    };

    // Constructor
    explicit Compressor(CompressorType type) : m_type(type) {}

    // Destructor
    virtual ~Compressor() = default;

    // Explicitly disable copy and move constructor/assignment
    Compressor(Compressor const&) = delete;

    Compressor& operator=(Compressor const&) = delete;

    // Methods
    /**
     * Closes the compression stream
     */
    virtual void close() = 0;

protected:
    CompressorType m_type;
};
}  // namespace clp_s

#endif  // CLP_S_COMPRESSOR_HPP
