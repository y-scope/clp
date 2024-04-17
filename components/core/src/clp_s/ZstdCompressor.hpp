// Code from CLP

#ifndef CLP_S_ZSTDCOMPRESSOR_HPP
#define CLP_S_ZSTDCOMPRESSOR_HPP

#include <memory>
#include <string>

#include <spdlog/spdlog.h>
#include <zstd.h>
#include <zstd_errors.h>

#include "Compressor.hpp"
#include "FileWriter.hpp"
#include "TraceableException.hpp"

namespace clp_s {
constexpr int cDefaultCompressionLevel = 3;

class ZstdCompressor : public Compressor {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructor
    ZstdCompressor();

    // Destructor
    ~ZstdCompressor() override;

    // Explicitly disable copy and move constructor/assignment
    ZstdCompressor(ZstdCompressor const&) = delete;

    ZstdCompressor& operator=(ZstdCompressor const&) = delete;

    // Methods implementing the WriterInterface
    /**
     * Writes the given data to the compressor
     * @param data
     * @param data_length
     */
    void write(char const* data, size_t data_length);

    /**
     * Writes the given numeric value to the compressor
     * @param val
     * @tparam ValueType
     */
    template <typename ValueType>
    void write_numeric_value(ValueType val) {
        write(reinterpret_cast<char*>(&val), sizeof(val));
    }

    /**
     * Writes the given string to the compressor
     * @param str
     */
    void write_string(std::string const& str) { write(str.c_str(), str.length()); }

    /**
     * Writes any internally buffered data to file and ends the current frame
     */
    void flush();

    // Methods implementing the Compressor interface
    /**
     * Closes the compressor
     */
    void close() override;

    /**
     * Initialize streaming compressor
     * @param file_writer
     * @param compression_level
     */
    void open(FileWriter& file_writer, int compression_level = cDefaultCompressionLevel);

private:
    // Variables
    FileWriter* m_compressed_stream_file_writer{};

    // Compressed stream variables
    ZSTD_CStream* m_compression_stream;
    bool m_compression_stream_contains_data;

    ZSTD_outBuffer m_compressed_stream_block{};
    std::unique_ptr<char[]> m_compressed_stream_block_buffer;

    size_t m_uncompressed_stream_pos{};
};
}  // namespace clp_s

#endif  // CLP_S_ZSTDCOMPRESSOR_HPP
