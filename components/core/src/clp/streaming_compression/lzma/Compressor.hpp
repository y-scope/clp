#ifndef CLP_STREAMING_COMPRESSION_LZMA_COMPRESSOR_HPP
#define CLP_STREAMING_COMPRESSION_LZMA_COMPRESSOR_HPP

#include <cstdint>
#include <cstddef>
#include <memory>

#include <lzma.h>
#include <zconf.h>

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

    class LzmaOption {
    public:
        LzmaOption()
                : m_compression_level{cDefaultCompressionLevel},
                  m_dict_size{cDefaultDictionarySize} {}

        auto set_compression_level(int compression_level) -> void {
            if (compression_level < cMinCompressionLevel) {
                m_compression_level = cMinCompressionLevel;
            } else if (compression_level > cMaxCompressionLevel) {
                m_compression_level = cMaxCompressionLevel;
            } else {
                m_compression_level = compression_level;
            }
        }

        auto set_dict_size(uint32_t dict_size) -> void { m_dict_size = dict_size; }

        [[nodiscard]] auto get_compression_level() const -> int { return m_compression_level; }

        [[nodiscard]] auto get_dict_size() const -> uint32_t { return m_dict_size; }

    private:
        int m_compression_level;
        uint32_t m_dict_size;
    };

    // Constructor
    Compressor();

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

    // Methods
    static auto set_compression_level(int compression_level) -> void {
        m_option.set_compression_level(compression_level);
    }

    static auto set_dict_size(uint32_t dict_size) -> void { m_option.set_dict_size(dict_size); }

private:
    using LzmaStream = lzma_stream;

    /**
     * Flushes the stream and closes it
     */
    void flush_and_close_compression_stream();

    static void init_lzma_encoder(LzmaStream* strm);
    static LzmaOption m_option;
    static constexpr size_t cCompressedStreamBlockBufferSize{4096};  // 4KiB

    // Variables
    FileWriter* m_compressed_stream_file_writer{nullptr};

    // Compressed stream variables
    std::unique_ptr<LzmaStream> m_compression_stream{std::make_unique<LzmaStream>()};
    bool m_compression_stream_contains_data{false};

    Array<Bytef> m_compressed_stream_block_buffer{cCompressedStreamBlockBufferSize};

    size_t m_uncompressed_stream_pos{0};
};
}  // namespace clp::streaming_compression::lzma

#endif  // CLP_STREAMING_COMPRESSION_LZMA_COMPRESSOR_HPP
