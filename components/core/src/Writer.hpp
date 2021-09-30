#ifndef WRITER_HPP
#define WRITER_HPP

// Project headers
#include "ErrorCode.hpp"
#include "PacketEncoder.hpp"
#include "TraceableException.hpp"

/**
 * C++ wrapper for write system call
 */
class Writer {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "Writer operation failed";
        }
    };

    // Methods
    /**
     * Tries to write a buffer of data to the file descriptor
     * @param fd
     * @param buf
     * @param buf_len Length of the buffer
     * @return ErrorCode_BadParam if file descriptor or buffer pointer are invalid
     * @return ErrorCode_errno if write failed
     * @return ErrorCode_Success otherwise
     */
    static ErrorCode try_write (int fd, const unsigned char* buf, size_t buf_len);

    /**
     * Tries to write a packet to the file descriptor
     * @param fd
     * @param packet_encoder
     * @return Same as Writer::try_write
     */
    static ErrorCode try_write_packet (int fd, PacketEncoder& packet_encoder);
    /**
     * Writes a packet to the file descriptor
     * @param fd
     * @param packet_encoder
     */
    static void write_packet (int fd, PacketEncoder& packet_encoder);
};

#endif //WRITER_HPP
