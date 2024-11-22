#ifndef CLP_STREAMING_COMPRESSION_DECOMPRESSOR_HPP
#define CLP_STREAMING_COMPRESSION_DECOMPRESSOR_HPP

#include <string>

#include "../FileReader.hpp"
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
        char const* what() const noexcept override {
            return "streaming_compression::Decompressor operation failed";
        }
    };

    // Constructor
    explicit Decompressor(CompressorType type) : m_compression_type(type) {}

    // Destructor
    ~Decompressor() = default;

    // Explicitly disable copy and move constructor/assignment
    Decompressor(Decompressor const&) = delete;
    Decompressor& operator=(Decompressor const&) = delete;

    // Methods
    /**
     * Initialize streaming decompressor to decompress from the specified compressed data buffer
     * @param compressed_data_buffer
     * @param compressed_data_buffer_size
     */
    virtual void open(char const* compressed_data_buffer, size_t compressed_data_buffer_size) = 0;
    /**
     * Initializes the decompressor to decompress from an open file
     * @param file_reader
     * @param file_read_buffer_capacity The maximum amount of data to read from a file at a time
     */
    virtual void open(FileReader& file_reader, size_t file_read_buffer_capacity) = 0;
    /**
     * Closes decompression stream
     */
    virtual void close() = 0;

    virtual ErrorCode get_decompressed_stream_region(
            size_t decompressed_stream_pos,
            char* extraction_buf,
            size_t extraction_len
    ) = 0;

protected:
    // Variables
    CompressorType m_compression_type;
};
}  // namespace clp::streaming_compression

#endif  // CLP_STREAMING_COMPRESSION_DECOMPRESSOR_HPP
