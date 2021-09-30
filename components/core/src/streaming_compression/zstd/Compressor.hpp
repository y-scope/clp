#ifndef STREAMING_COMPRESSION_ZSTD_COMPRESSOR_HPP
#define STREAMING_COMPRESSION_ZSTD_COMPRESSOR_HPP

// C++ standard libraries
#include <memory>
#include <string>

// Zstd library
#include <zstd.h>
#include <zstd_errors.h>

// Project headers
#include "../../FileWriter.hpp"
#include "../../TraceableException.hpp"
#include "../Compressor.hpp"
#include "Constants.hpp"

namespace streaming_compression { namespace zstd {
    class Compressor : public ::streaming_compression::Compressor {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException(error_code, filename, line_number) {}

            // Methods
            const char* what () const noexcept override {
                return "streaming_compression::zstd::Compressor operation failed";
            }
        };

        // Constructor
        Compressor ();

        // Destructor
        ~Compressor ();

        // Explicitly disable copy and move constructor/assignment
        Compressor (const Compressor&) = delete;
        Compressor& operator = (const Compressor&) = delete;

        // Methods implementing the WriterInterface
        /**
         * Writes the given data to the compressor
         * @param data
         * @param data_length
         */
        void write (const char* data, size_t data_length) override;
        /**
         * Writes any internally buffered data to file and ends the current frame
         */
        void flush () override;

        /**
         * Tries to get the current position of the write head
         * @param pos Position of the write head
         * @return ErrorCode_NotInit if the compressor is not open
         * @return ErrorCode_Success on success
         */
        ErrorCode try_get_pos (size_t& pos) const override;

        // Methods implementing the Compressor interface
        /**
         * Closes the compressor
         */
        void close () override;

        // Methods
        /**
         * Initialize streaming compressor
         * @param file_writer
         * @param compression_level
         */
        void open (FileWriter& file_writer, int compression_level = cDefaultCompressionLevel);

        /**
         * Flushes the stream without ending the current frame
         */
        void flush_without_ending_frame ();

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
}}

#endif // STREAMING_COMPRESSION_ZSTD_COMPRESSOR_HPP