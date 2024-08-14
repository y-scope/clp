#include "SysFileReader.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <boost/filesystem.hpp>

using std::string;

namespace clp {
    auto SysFileReader::try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read)
            -> ErrorCode {
        if (nullptr == buf) {
            return ErrorCode_BadParam;
        }

        num_bytes_read = 0;
        while (true) {
            auto const bytes_read = ::read(m_fd.get_raw_fd(), buf, num_bytes_to_read);
            if (0 == bytes_read) {
                break;
            }
            if (bytes_read < 0) {
                return ErrorCode_errno;
            }

            buf += bytes_read;
            num_bytes_read += bytes_read;
            num_bytes_to_read -= bytes_read;
            if (num_bytes_read == num_bytes_to_read) {
                return ErrorCode_Success;
            }
        }
        if (0 == num_bytes_read) {
            return ErrorCode_EndOfFile;
        }
        return ErrorCode_Success;
    }

    auto SysFileReader::try_seek_from_begin(size_t pos) -> ErrorCode {
        if (auto const offset = lseek(m_fd.get_raw_fd(), static_cast<off_t>(pos), SEEK_SET);
            -1 == offset)
        {
            return ErrorCode_errno;
        }

        return ErrorCode_Success;
    }

    auto SysFileReader::try_get_pos(size_t& pos) -> ErrorCode {
        pos = lseek(m_fd.get_raw_fd(), 0, SEEK_CUR);
        if (static_cast<off_t>(-1) == pos) {
            return ErrorCode_errno;
        }

        return ErrorCode_Success;
    }

    auto SysFileReader::try_fstat(struct stat& stat_buffer) const -> ErrorCode {
        if (auto const return_value = fstat(m_fd.get_raw_fd(), &stat_buffer); 0 != return_value) {
            return ErrorCode_errno;
        }
        return ErrorCode_Success;
    }
}  // namespace clp
