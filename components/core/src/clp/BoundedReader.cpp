#include "BoundedReader.hpp"

#include <cstddef>

#include "ErrorCode.hpp"

namespace clp {
auto BoundedReader::try_seek_from_begin(size_t pos) -> ErrorCode {
    auto const next_pos = pos > m_bound ? m_bound : pos;
    if (auto const rc = m_reader->try_seek_from_begin(next_pos); ErrorCode_Success != rc) {
        m_curr_pos = ErrorCode_EndOfFile == rc ? next_pos : m_curr_pos;
        return rc;
    }
    m_curr_pos = next_pos;
    if (m_curr_pos >= m_bound) {
        return ErrorCode_EndOfFile;
    }
    return ErrorCode_Success;
}

auto BoundedReader::try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read)
        -> ErrorCode {
    if (m_curr_pos == m_bound) {
        num_bytes_read = 0;
        return ErrorCode_EndOfFile;
    }

    if ((m_curr_pos + num_bytes_to_read) > m_bound) {
        num_bytes_to_read = m_bound - m_curr_pos;
    }

    auto const rc = m_reader->try_read(buf, num_bytes_to_read, num_bytes_read);
    m_curr_pos += num_bytes_read;
    if (ErrorCode_EndOfFile == rc) {
        if (0 == num_bytes_read) {
            return ErrorCode_EndOfFile;
        }
    } else if (ErrorCode_Success != rc) {
        return rc;
    }
    return ErrorCode_Success;
}
}  // namespace clp
