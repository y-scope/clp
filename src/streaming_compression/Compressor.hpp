#ifndef STREAMING_COMPRESSION_COMPRESSOR_HPP
#define STREAMING_COMPRESSION_COMPRESSOR_HPP

// C++ libraries
#include <cstdint>
#include <string>

// Project headers
#include "../TraceableException.hpp"
#include "../WriterInterface.hpp"
#include "Constants.hpp"

namespace streaming_compression {
    class Compressor : public WriterInterface {
    public:
        // Types
        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

            // Methods
            const char* what () const noexcept override {
                return "streaming_compression::Compressor operation failed";
            }
        };

        // Constructor
        explicit Compressor (CompressorType type) : m_type(type) {}

        // Destructor
        virtual ~Compressor () = default;

        // Explicitly disable copy and move constructor/assignment
        Compressor (const Compressor&) = delete;
        Compressor& operator = (const Compressor&) = delete;

        // Methods implementing the WriterInterface
        /**
         * Unsupported operation
         * @param pos
         * @return ErrorCode_Unsupported
         */
        ErrorCode try_seek_from_begin (size_t pos) override { return ErrorCode_Unsupported; };
        /**
         * Unsupported operation
         * @param pos
         * @return ErrorCode_Unsupported
         */
        ErrorCode try_seek_from_current (off_t offset) override { return ErrorCode_Unsupported; };

        // Methods
        /**
         * Closes the compression stream
         */
        virtual void close () = 0;

    protected:
        // Variables
        CompressorType m_type;
    };
}

#endif //STREAMING_COMPRESSION_COMPRESSOR_HPP
