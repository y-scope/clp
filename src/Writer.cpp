#include "Writer.hpp"

// C standard libraries
#include <unistd.h>

// Project headers
#include "Defs.h"

ErrorCode Writer::try_write (int fd, const unsigned char* buf, size_t buf_len) {
    if (fd < 0 || nullptr == buf) {
        return ErrorCode_BadParam;
    }

    while (buf_len > 0) {
        ssize_t num_bytes_sent = ::write(fd, buf, buf_len);
        if (-1 == num_bytes_sent) {
            return ErrorCode_errno;
        }
        buf_len -= num_bytes_sent;
    }

    return ErrorCode_Success;
}

ErrorCode Writer::try_write_packet (int fd, PacketEncoder& packet_encoder) {
    return try_write(fd, packet_encoder.get_packet(), packet_encoder.get_packet_len());
}

void Writer::write_packet(int fd, PacketEncoder &packet_encoder) {
    ErrorCode error_code = try_write_packet(fd, packet_encoder);
    if (ErrorCode_Success != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
}
