#ifndef STREAMING_COMPRESSION_LZMA_COMPRESSOR_HPP
#define STREAMING_COMPRESSION_LZMA_COMPRESSOR_HPP

// C++ standard libraries
#include <memory>
#include <string>

// ZLIB library
#include <lzma.h>
#include <zlib.h>

// Project headers
#include "../../FileWriter.hpp"
#include "../../TraceableException.hpp"
#include "../Compressor.hpp"
#include "Constants.hpp"

namespace streaming_compression::lzma {
class Compressor : public ::streaming_compression::Compressor {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "streaming_compression::gzip::Compressor operation failed";
        }
    };

    class LzmaOption {
    public:
        LzmaOption()
                : m_compression_level{cDefaultCompressionLevel},
                  m_dict_size{cDefaultDictionarySize} {}

        auto set_compression_level(int compression_level) -> void {
            if (0 > compression_level) {
                m_compression_level = 0;
            } else if (9 < compression_level) {
                m_compression_level = 9;
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
    ~Compressor();

    // Explicitly disable copy and move constructor/assignment
    Compressor(Compressor const&) = delete;
    Compressor& operator=(Compressor const&) = delete;

    // Methods implementing the WriterInterface
    /**
     * Writes the given data to the compressor
     * @param data
     * @param data_length
     */
    void write(char const* data, size_t data_length) override;
    /**
     * Writes any internally buffered data to file and ends the current frame
     */
    void flush() override;

    /**
     * Tries to get the current position of the write head
     * @param pos Position of the write head
     * @return ErrorCode_NotInit if the compressor is not open
     * @return ErrorCode_Success on success
     */
    ErrorCode try_get_pos(size_t& pos) const override;

    // Methods implementing the Compressor interface
    /**
     * Initialize streaming compressor
     * @param file_writer
     * @param compression_level
     */
    void open(FileWriter& file_writer, int compression_level) override;

    /**
     * Closes the compressor
     */
    void close() override;

    // Methods
    static auto set_compression_level(int compression_level) -> void {
        m_option.set_compression_level(compression_level);
    }

    static auto set_dict_size(uint32_t dict_size) -> void { m_option.set_dict_size(dict_size); }

private:
    /**
     * Flushes the stream and closes it
     */
    void flush_and_close_compression_stream();

    static void init_lzma_encoder(lzma_stream* strm);
    static LzmaOption m_option;

    // Variables
    FileWriter* m_compressed_stream_file_writer;

    // Compressed stream variables
    lzma_stream* m_compression_stream;
    bool m_compression_stream_contains_data;

    std::unique_ptr<Bytef[]> m_compressed_stream_block_buffer;

    size_t m_uncompressed_stream_pos;
};
}  // namespace streaming_compression::lzma

#endif  // STREAMING_COMPRESSION_LZMA_COMPRESSOR_HPP
