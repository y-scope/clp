#ifndef CLP_STREAMING_COMPRESSION_COMPRESSOR_HPP
#define CLP_STREAMING_COMPRESSION_COMPRESSOR_HPP

#include <sys/types.h>

#include <cstddef>

#include "../ErrorCode.hpp"
#include "../FileWriter.hpp"
#include "../TraceableException.hpp"
#include "../WriterInterface.hpp"

namespace clp::streaming_compression {
/**
 * Abstract compressor interface.
 */
class Compressor : public WriterInterface {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException{error_code, filename, line_number} {}

        // Methods
        [[nodiscard]] auto what() const noexcept -> char const* override {
            return "streaming_compression::Compressor operation failed";
        }
    };

    // Constructor
    Compressor() = default;

    // Destructor
    virtual ~Compressor() = default;

    // Delete copy constructor and assignment operator
    Compressor(Compressor const&) = delete;
    auto operator=(Compressor const&) -> Compressor& = delete;

    // Default move constructor and assignment operator
    Compressor(Compressor&&) noexcept = default;
    auto operator=(Compressor&&) noexcept -> Compressor& = default;

    // Methods implementing the WriterInterface
    /**
     * Unsupported operation
     * @param pos
     * @return ErrorCode_Unsupported
     */
    [[nodiscard]] auto try_seek_from_begin([[maybe_unused]] size_t pos) -> ErrorCode override {
        return ErrorCode_Unsupported;
    }

    /**
     * Unsupported operation
     * @param pos
     * @return ErrorCode_Unsupported
     */
    [[nodiscard]] auto try_seek_from_current([[maybe_unused]] off_t offset) -> ErrorCode override {
        return ErrorCode_Unsupported;
    }

    // Methods
    /**
     * Closes the compressor
     */
    virtual auto close() -> void = 0;

    /**
     * Initializes the compression stream
     * @param file_writer
     */
    virtual auto open(FileWriter& file_writer) -> void = 0;
};
}  // namespace clp::streaming_compression

#endif  // CLP_STREAMING_COMPRESSION_COMPRESSOR_HPP
