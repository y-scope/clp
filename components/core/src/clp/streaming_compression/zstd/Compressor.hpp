#ifndef CLP_STREAMING_COMPRESSION_ZSTD_COMPRESSOR_HPP
#define CLP_STREAMING_COMPRESSION_ZSTD_COMPRESSOR_HPP

#include <cstddef>
#include <vector>

#include <zstd.h>

#include "../../ErrorCode.hpp"
#include "../../FileWriter.hpp"
#include "../../TraceableException.hpp"
#include "../Compressor.hpp"
#include "Constants.hpp"

namespace clp::streaming_compression::zstd {
class Compressor : public ::clp::streaming_compression::Compressor {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        [[nodiscard]] auto what() const noexcept -> char const* override {
            return "streaming_compression::zstd::Compressor operation failed";
        }
    };

    // Constructor
    Compressor();

    // Destructor
    ~Compressor() override;

    // Explicitly disable copy constructor/assignment and enable the move version
    Compressor(Compressor const&) = delete;
    auto operator=(Compressor const&) -> Compressor& = delete;

    Compressor(Compressor&&) noexcept = default;
    auto operator=(Compressor&&) -> Compressor& = default;

    // Methods implementing the WriterInterface
    /**
     * Writes the given data to the compressor
     * @param data
     * @param data_length
     */
    auto write(char const* data, size_t data_length) -> void override;
    /**
     * Writes any internally buffered data to file and ends the current frame
     */
    auto flush() -> void override;

    /**
     * Tries to get the current position of the write head
     * @param pos Position of the write head
     * @return ErrorCode_NotInit if the compressor is not open
     * @return ErrorCode_Success on success
     */
    [[nodiscard]] auto try_get_pos(size_t& pos) const -> ErrorCode override;

    // Methods implementing the Compressor interface
    /**
     * Closes the compressor
     */
    auto close() -> void override;

    /**
     * Initializes the compression stream with the given compression level
     * @param file_writer
     * @param compression_level
     */
    auto open(FileWriter& file_writer, int compression_level = cDefaultCompressionLevel)
            -> void override;

    /**
     * Flushes the stream without ending the current frame
     */
    auto flush_without_ending_frame() -> void;

private:
    // Variables
    FileWriter* m_compressed_stream_file_writer;

    // Compressed stream variables
    ZSTD_CStream* m_compression_stream;
    bool m_compression_stream_contains_data;

    ZSTD_outBuffer m_compressed_stream_block;
    std::vector<char> m_compressed_stream_block_buffer;

    size_t m_uncompressed_stream_pos;

    /**
     * Tells if a `size_t` ZStd function result is an error code and is not `ZSTD_error_no_error`
     */
    [[nodiscard]] static auto zstd_is_error(size_t size_t_function_result) -> bool;
};
}  // namespace clp::streaming_compression::zstd

#endif  // CLP_STREAMING_COMPRESSION_ZSTD_COMPRESSOR_HPP
