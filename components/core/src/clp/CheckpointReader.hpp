#ifndef CLP_CHECKPOINTREADER_HPP
#define CLP_CHECKPOINTREADER_HPP

#include "ReaderInterface.hpp"

namespace clp {
/**
 * CheckpointReader is a ReaderInterface designed to wrap other ReaderInterfaces and prevent users
 * from reading or seeking beyond a certain point in the underlying input stream.
 *
 * This is useful when the underlying input stream is divided into several logical segments and we
 * want to prevent a reader for an earlier segment consuming any bytes from a later segment. In
 * particular, reading part of a later segment may force the reader for that later segment to seek
 * backwards, which can be either inefficient or impossible for certain kinds of input streams.
 */
class CheckpointReader : public ReaderInterface {
public:
    // Constructor
    explicit CheckpointReader(ReaderInterface* reader, size_t checkpoint)
            : m_reader(reader),
              m_checkpoint(checkpoint) {
        if (nullptr == m_reader) {
            throw ReaderInterface::OperationFailed(ErrorCode_BadParam, __FILE__, __LINE__);
        }
        m_cur_pos = m_reader->get_pos();
        if (m_cur_pos > m_checkpoint) {
            throw ReaderInterface::OperationFailed(ErrorCode_BadParam, __FILE__, __LINE__);
        }
    }

    // Methods implementing the ReaderInterface
    /**
     * Tries to get the current position of the read head in the underlying reader.
     * @param pos Position of the read head in the underlying reader
     * @return ErrorCode_Success on success
     * @return ErrorCode_errno on failure
     */
    auto try_get_pos(size_t& pos) -> ErrorCode override { return m_reader->try_get_pos(pos); }

    /**
     * Tries to seek to the given position, limited by the checkpoint. If pos is past the checkpoint
     * then this reader seeks up to the checkpoint in the underlying stream, then returns
     * ErrorCode_EndOfFile on success or ErrorCode_errno otherwise.
     * @param pos
     * @return ErrorCode_Success on success
     * @return ErrorCode_EndOfFile on EOF or if trying to seek beyond the checkpoint
     * @return ErrorCode_errno on failure
     */
    auto try_seek_from_begin(size_t pos) -> ErrorCode override;

    /**
     * Tries to read up to a given number of bytes from the file, limited by the checkpoint. If the
     * requested read would advance the read head beyond the checkpoint then the read size is
     * adjusted such that the read head is advanced to the checkpoint. If a read is attempted when
     * the read head is at the checkpoint then ErrorCode_EndOfFile is returned.
     * @param buf
     * @param num_bytes_to_read The number of bytes to try and read
     * @param num_bytes_read The actual number of bytes read
     * @return ErrorCode_errno on error
     * @return ErrorCode_EndOfFile on EOF or trying to read after hitting checkpoint
     * @return ErrorCode_Success on success
     */
    auto
    try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) -> ErrorCode override;

    auto try_read_to_delimiter(char delim, bool keep_delimiter, bool append, std::string& str)
            -> ErrorCode override {
        return ErrorCode_Unsupported;
    }

private:
    ReaderInterface* m_reader{nullptr};
    size_t m_checkpoint{};
    size_t m_cur_pos{};
};
}  // namespace clp

#endif  // CLP_CHECKPOINTREADER_HPP
