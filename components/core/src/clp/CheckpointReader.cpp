#include "CheckpointReader.hpp"

namespace clp {
auto CheckpointReader::try_seek_from_begin(size_t pos) -> ErrorCode {
    m_cur_pos = pos > m_checkpoint ? m_checkpoint : pos;
    if (auto const rc = m_reader->try_seek_from_begin(m_cur_pos);
        ErrorCode_Success != rc) {
        return rc;
    }
    if (m_cur_pos >= m_checkpoint) {
        return ErrorCode_EndOfFile;
    }
    return ErrorCode_Success;
}

auto CheckpointReader::try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read)
        -> ErrorCode {
    if (m_cur_pos == m_checkpoint) {
        return ErrorCode_EndOfFile;
    }
    
    if ((m_cur_pos + num_bytes_to_read) > m_checkpoint) {
        num_bytes_to_read = m_checkpoint - m_cur_pos;
    }

    auto const rc = m_reader->try_read(buf, num_bytes_to_read, num_bytes_read);
    m_cur_pos += num_bytes_read;
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
