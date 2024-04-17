#ifndef GLT_STREAMING_COMPRESSION_COMPRESSOR_HPP
#define GLT_STREAMING_COMPRESSION_COMPRESSOR_HPP

#include <cstdint>
#include <string>

#include "../TraceableException.hpp"
#include "../WriterInterface.hpp"
#include "Constants.hpp"

namespace glt::streaming_compression {
class Compressor : public WriterInterface {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "streaming_compression::Compressor operation failed";
        }
    };

    // Constructor
    explicit Compressor(CompressorType type) : m_type(type) {}

    // Destructor
    virtual ~Compressor() = default;

    // Explicitly disable copy and move constructor/assignment
    Compressor(Compressor const&) = delete;
    Compressor& operator=(Compressor const&) = delete;

    // Methods implementing the WriterInterface
    /**
     * Unsupported operation
     * @param pos
     * @return ErrorCode_Unsupported
     */
    ErrorCode try_seek_from_begin(size_t pos) override { return ErrorCode_Unsupported; }

    /**
     * Unsupported operation
     * @param pos
     * @return ErrorCode_Unsupported
     */
    ErrorCode try_seek_from_current(off_t offset) override { return ErrorCode_Unsupported; }

    // Methods
    /**
     * Closes the compression stream
     */
    virtual void close() = 0;

protected:
    // Variables
    CompressorType m_type;
};
}  // namespace glt::streaming_compression

#endif  // GLT_STREAMING_COMPRESSION_COMPRESSOR_HPP
