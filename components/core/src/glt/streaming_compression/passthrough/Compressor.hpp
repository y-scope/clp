#ifndef GLT_STREAMING_COMPRESSION_PASSTHROUGH_COMPRESSOR_HPP
#define GLT_STREAMING_COMPRESSION_PASSTHROUGH_COMPRESSOR_HPP

#include "../../FileWriter.hpp"
#include "../../TraceableException.hpp"
#include "../Compressor.hpp"

namespace glt::streaming_compression::passthrough {
/**
 * Compressor that passes all data through without any compression.
 */
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
            return "streaming_compression::passthrough::Compressor operation failed";
        }
    };

    // Constructors
    Compressor()
            : ::glt::streaming_compression::Compressor(CompressorType::Passthrough),
              m_compressed_stream_file_writer(nullptr) {}

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
     * Flushes any buffered data
     */
    void flush() override;
    /**
     * Tries to get the current position of the write head
     * @param pos Position of the write head
     * @return ErrorCode_NotInit if the compressor is not open
     * @return Same as FileWriter::try_get_pos
     */
    ErrorCode try_get_pos(size_t& pos) const override;

    // Methods implementing the Compressor interface
    /**
     * Closes the compressor
     */
    void close() override;

    // Methods
    /**
     * Initializes the compressor
     * @param file_writer
     */
    void open(FileWriter& file_writer);

private:
    // Variables
    FileWriter* m_compressed_stream_file_writer;
};
}  // namespace glt::streaming_compression::passthrough

#endif  // GLT_STREAMING_COMPRESSION_PASSTHROUGH_COMPRESSOR_HPP
