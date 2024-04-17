#ifndef GLT_STREAMING_COMPRESSION_ZSTD_COMPRESSOR_HPP
#define GLT_STREAMING_COMPRESSION_ZSTD_COMPRESSOR_HPP

#include <memory>
#include <string>

#include <zstd.h>
#include <zstd_errors.h>

#include "../../FileWriter.hpp"
#include "../../TraceableException.hpp"
#include "../Compressor.hpp"
#include "Constants.hpp"

namespace glt::streaming_compression::zstd {
class Compressor : public ::glt::streaming_compression::Compressor {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "streaming_compression::zstd::Compressor operation failed";
        }
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
     * Closes the compressor
     */
    void close() override;

    // Methods
    /**
     * Initialize streaming compressor
     * @param file_writer
     * @param compression_level
     */
    void open(FileWriter& file_writer, int compression_level = cDefaultCompressionLevel);

    /**
     * Flushes the stream without ending the current frame
     */
    void flush_without_ending_frame();

private:
    // Variables
    FileWriter* m_compressed_stream_file_writer;

    // Compressed stream variables
    ZSTD_CStream* m_compression_stream;
    bool m_compression_stream_contains_data;

    ZSTD_outBuffer m_compressed_stream_block;
    std::unique_ptr<char[]> m_compressed_stream_block_buffer;

    size_t m_uncompressed_stream_pos;
};
}  // namespace glt::streaming_compression::zstd

#endif  // GLT_STREAMING_COMPRESSION_ZSTD_COMPRESSOR_HPP
