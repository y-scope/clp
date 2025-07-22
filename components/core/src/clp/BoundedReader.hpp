#ifndef CLP_BOUNDEDREADER_HPP
#define CLP_BOUNDEDREADER_HPP

#include <cstddef>
#include <string>

#include "ErrorCode.hpp"
#include "ReaderInterface.hpp"

namespace clp {
/**
 * BoundedReader is a ReaderInterface designed to wrap other ReaderInterfaces and prevent users
 * from reading or seeking beyond a certain point in the underlying input stream.
 *
 * This is useful when the underlying input stream is divided into several logical segments and we
 * want to prevent a reader for an earlier segment consuming any bytes from a later segment. In
 * particular, reading part of a later segment may force the reader for that later segment to seek
 * backwards, which can be either inefficient or impossible for certain kinds of input streams.
 */
class BoundedReader : public ReaderInterface {
public:
    // Constructor
    explicit BoundedReader(ReaderInterface* reader, size_t bound)
            : m_reader{reader},
              m_bound{bound} {
        if (nullptr == m_reader) {
            throw ReaderInterface::OperationFailed(ErrorCode_BadParam, __FILE__, __LINE__);
        }
        m_curr_pos = m_reader->get_pos();
        if (m_curr_pos > m_bound) {
            throw ReaderInterface::OperationFailed(ErrorCode_BadParam, __FILE__, __LINE__);
        }
    }

    // Methods implementing the ReaderInterface
    /**
     * Tries to get the current position of the read head in the underlying reader.
     * @param pos Returns the position of the underlying reader's head
     * @return ErrorCode_Success on success
     * @return ErrorCode_errno on failure
     */
    [[nodiscard]] auto try_get_pos(size_t& pos) -> ErrorCode override {
        return m_reader->try_get_pos(pos);
    }

    /**
     * Tries to seek to the given position, limited by the bound.
     * @param pos
     * @return ErrorCode_Success on success
     * @return ErrorCode_EndOfFile on EOF or if trying to seek beyond the checkpoint
     * @return ErrorCode_errno on failure
     */
    [[nodiscard]] auto try_seek_from_begin(size_t pos) -> ErrorCode override;

    /**
     * Tries to read up to a given number of bytes from the file, limited by the bound.
     * @param buf
     * @param num_bytes_to_read The number of bytes to try and read
     * @param num_bytes_read The actual number of bytes read
     * @return ErrorCode_errno on error
     * @return ErrorCode_EndOfFile on EOF or trying to read after hitting checkpoint
     * @return ErrorCode_Success on success
     */
    [[nodiscard]] auto try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read)
            -> ErrorCode override;

    /**
     * This function is unsupported because BoundedReader can not delegate to a potentially
     * efficient implementation in the underlying reader, as the underlying reader's implementation
     * will not respect the bound.
     * @return ErrorCode_Unsupported
     */
    [[nodiscard]] auto try_read_to_delimiter(
            [[maybe_unused]] char delim,
            [[maybe_unused]] bool keep_delimiter,
            [[maybe_unused]] bool append,
            [[maybe_unused]] std::string& str
    ) -> ErrorCode override {
        return ErrorCode_Unsupported;
    }

private:
    ReaderInterface* m_reader{nullptr};
    size_t m_bound{};
    size_t m_curr_pos{};
};
}  // namespace clp

#endif  // CLP_BOUNDEDREADER_HPP
