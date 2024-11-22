#ifndef CLP_STREAMING_COMPRESSION_PASSTHROUGH_COMPRESSOR_HPP
#define CLP_STREAMING_COMPRESSION_PASSTHROUGH_COMPRESSOR_HPP

#include <cstddef>

#include "../../ErrorCode.hpp"
#include "../../FileWriter.hpp"
#include "../../TraceableException.hpp"
#include "../Compressor.hpp"

namespace clp::streaming_compression::passthrough {
/**
 * Compressor that passes all data through without any compression.
 */
class Compressor : public ::clp::streaming_compression::Compressor {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException{error_code, filename, line_number} {}

        // Methods
        [[nodiscard]] auto what() const noexcept -> char const* override {
            return "streaming_compression::passthrough::Compressor operation failed";
        }
    };

    // Constructor
    Compressor() = default;

    // Destructor
    ~Compressor() override = default;

    // Delete copy constructor and assignment operator
    Compressor(Compressor const&) = delete;
    auto operator=(Compressor const&) -> Compressor& = delete;

    // Default move constructor and assignment operator
    Compressor(Compressor&&) noexcept = default;
    auto operator=(Compressor&&) noexcept -> Compressor& = default;

    // Methods implementing the WriterInterface
    /**
     * Writes the given data to the compressor
     * @param data
     * @param data_length
     */
    auto write(char const* data, size_t data_length) -> void override;

    /**
     * Flushes any buffered data
     */
    auto flush() -> void override;

    /**
     * Tries to get the current position of the write head
     * @param pos Position of the write head
     * @return ErrorCode_NotInit if the compressor is not open
     * @return Same as FileWriter::try_get_pos
     */
    [[nodiscard]] auto try_get_pos(size_t& pos) const -> ErrorCode override;

    // Methods implementing the Compressor interface
    /**
     * Closes the compressor
     */
    auto close() -> void override;

    /**
     * Initializes the compression stream
     * @param file_writer
     */
    auto open(FileWriter& file_writer) -> void override;

private:
    // Variables
    FileWriter* m_compressed_stream_file_writer{nullptr};
};
}  // namespace clp::streaming_compression::passthrough

#endif  // CLP_STREAMING_COMPRESSION_PASSTHROUGH_COMPRESSOR_HPP
