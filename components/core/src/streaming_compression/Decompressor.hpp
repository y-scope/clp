#ifndef STREAMING_COMPRESSION_DECOMPRESSOR_HPP
#define STREAMING_COMPRESSION_DECOMPRESSOR_HPP

// C++ libraries
#include <string>

// Project headers
#include "../ReaderInterface.hpp"
#include "../TraceableException.hpp"
#include "Constants.hpp"

namespace streaming_compression {
    class Decompressor : public ReaderInterface {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

            // Methods
            const char* what () const noexcept override {
                return "streaming_compression::Decompressor operation failed";
            }
        };

        // Constructor
        explicit Decompressor (CompressorType type) : m_compression_type(type) {}

        // Destructor
        ~Decompressor () = default;

        // Explicitly disable copy and move constructor/assignment
        Decompressor (const Decompressor&) = delete;
        Decompressor& operator = (const Decompressor&) = delete;

        // Methods
        /**
         * Closes decompression stream
         */
        virtual void close () = 0;

        virtual ErrorCode get_decompressed_stream_region (size_t decompressed_stream_pos, char* extraction_buf, size_t extraction_len) = 0;

    protected:
        // Variables
        CompressorType m_compression_type;
    };
}

#endif //STREAMING_COMPRESSION_DECOMPRESSOR_HPP
