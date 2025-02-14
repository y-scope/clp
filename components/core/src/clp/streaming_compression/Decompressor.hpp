#ifndef CLP_STREAMING_COMPRESSION_DECOMPRESSOR_HPP
#define CLP_STREAMING_COMPRESSION_DECOMPRESSOR_HPP

#include <string>

#include "../ReaderInterface.hpp"
#include "../TraceableException.hpp"
#include "Constants.hpp"

namespace clp::streaming_compression {
class Decompressor : public ReaderInterface {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        [[nodiscard]] auto what() const noexcept -> char const* override {
            return "streaming_compression::Decompressor operation failed";
        }
    };

    // Constructor
    explicit Decompressor(CompressorType type) : m_compression_type(type) {}

    // Destructor
    ~Decompressor() override = default;

    // Delete copy constructor and assignment operator
    Decompressor(Decompressor const&) = delete;
    auto operator=(Decompressor const&) -> Decompressor& = delete;

    // Default move constructor and assignment operator
    Decompressor(Decompressor&&) noexcept = default;
    auto operator=(Decompressor&&) noexcept -> Decompressor& = default;

    // Methods
    /**
     * Initialize streaming decompressor to decompress from the specified compressed data buffer
     * @param compressed_data_buffer
     * @param compressed_data_buffer_size
     */
    virtual auto open(char const* compressed_data_buffer, size_t compressed_data_buffer_size)
            -> void
            = 0;
    /**
     * Initializes the decompressor to decompress from a reader interface
     * @param reader
     * @param read_buffer_capacity The maximum amount of data to read from a reader at a time
     */
    virtual auto open(ReaderInterface& reader, size_t read_buffer_capacity) -> void = 0;
    /**
     * Closes decompression stream
     */
    virtual auto close() -> void = 0;

    [[nodiscard]] virtual auto get_decompressed_stream_region(
            size_t decompressed_stream_pos,
            char* extraction_buf,
            size_t extraction_len
    ) -> ErrorCode
            = 0;

protected:
    // Variables
    CompressorType m_compression_type;
};
}  // namespace clp::streaming_compression

#endif  // CLP_STREAMING_COMPRESSION_DECOMPRESSOR_HPP
