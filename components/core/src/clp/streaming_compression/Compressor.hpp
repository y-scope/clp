#ifndef CLP_STREAMING_COMPRESSION_COMPRESSOR_HPP
#define CLP_STREAMING_COMPRESSION_COMPRESSOR_HPP

#include <sys/types.h>

#include <cstddef>

#include "../ErrorCode.hpp"
#include "../FileWriter.hpp"
#include "../TraceableException.hpp"
#include "../WriterInterface.hpp"
#include "Constants.hpp"

namespace clp::streaming_compression {
/**
 * Generic compressor interface.
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
    explicit Compressor(CompressorType type) : m_type{type} {}

    // Destructor
    virtual ~Compressor() = default;

    // Explicitly disable copy constructor/assignment and enable the move version
    Compressor(Compressor const&) = delete;
    auto operator=(Compressor const&) -> Compressor& = delete;

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
     * Closes the compression stream
     */
    virtual auto close() -> void = 0;

    /**
     * Initializes the compression stream
     * @param file_writer
     */
    virtual auto open(FileWriter& file_writer) -> void { open(file_writer, 0); }

    /**
     * Initializes the compression stream with the given compression level
     * @param file_writer
     * @param compression_level
     */
    virtual auto open(FileWriter& file_writer, [[maybe_unused]] int const compression_level) -> void
                                                                                                = 0;

private:
    // Variables
    CompressorType m_type;
};
}  // namespace clp::streaming_compression

#endif  // CLP_STREAMING_COMPRESSION_COMPRESSOR_HPP
