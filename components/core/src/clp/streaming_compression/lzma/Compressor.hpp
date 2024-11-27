#ifndef CLP_STREAMING_COMPRESSION_LZMA_COMPRESSOR_HPP
#define CLP_STREAMING_COMPRESSION_LZMA_COMPRESSOR_HPP

#include <zconf.h>

#include <cstddef>
#include <memory>

#include <lzma.h>

#include "../../Array.hpp"
#include "../../ErrorCode.hpp"
#include "../../FileWriter.hpp"
#include "../../TraceableException.hpp"
#include "../Compressor.hpp"
#include "Constants.hpp"

namespace clp::streaming_compression::lzma {
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
            return "streaming_compression::lzma::Compressor operation failed";
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
     * Writes any internally buffered data to file and ends the current frame
     */
    auto flush() -> void override;

    /**
     * Tries to get the current position of the write head
     * @param pos Position of the write head
     * @return ErrorCode_NotInit if the compressor is not open
     * @return ErrorCode_Success on success
     */
    auto try_get_pos(size_t& pos) const -> ErrorCode override;

    /**
     * Closes the compressor
     */
    auto close() -> void override;

    // Methods implementing the Compressor interface
    /**
     * Initializes the compression stream with the default compression level
     * @param file_writer
     */
    auto open(FileWriter& file_writer) -> void override {
        this->open(file_writer, cDefaultCompressionLevel);
    }

    /**
     * Initializes the compression stream with the given compression level
     * @param file_writer
     * @param compression_level
     */
    auto open(FileWriter& file_writer, int compression_level) -> void;

private:
    using LzmaAction = lzma_action;
    using LzmaFilter = lzma_filter;
    using LzmaOptionsLzma = lzma_options_lzma;
    using LzmaStream = lzma_stream;

    /**
     * Initialize the Lzma compression stream
     * @param strm A pre-allocated `lzma_stream` object
     * @param compression_level
     * @param dict_size Dictionary size that indicates how many bytes of the
     *                  recently processed uncompressed data is kept in memory
     */
    static auto
    init_lzma_encoder(LzmaStream* strm, int compression_level, size_t dict_size) -> void;
    static constexpr size_t cCompressedStreamBlockBufferSize{4096};  // 4KiB

    /**
     * Flushes the stream and closes it
     */
    auto flush_and_close_compression_stream() -> void;

    /**
     * Repeatedly invoke lzma_code() compression workflow until LZMA_STREAM_END
     * is reached.
     * The workflow action needs to be kept the same throughout this process.
     * See also: https://github.com/tukaani-project/xz/blob/master/src/liblzma/api/lzma/base.h#L274
     *
     * @param action
     */
    auto compress(lzma_action action) -> void;

    /**
     * Pipes the current compressed data in the lzma buffer to the output file
     * and reset the compression buffer to receive new data.
     */
    auto pipe_data() -> void;

    // Variables
    FileWriter* m_compressed_stream_file_writer{nullptr};

    // Compressed stream variables
    std::unique_ptr<LzmaStream> m_compression_stream{std::make_unique<LzmaStream>()};
    bool m_compression_stream_contains_data{false};
    size_t m_dict_size{cDefaultDictionarySize};

    Array<Bytef> m_compressed_stream_block_buffer{cCompressedStreamBlockBufferSize};

    size_t m_uncompressed_stream_pos{0};
};
}  // namespace clp::streaming_compression::lzma

#endif  // CLP_STREAMING_COMPRESSION_LZMA_COMPRESSOR_HPP
