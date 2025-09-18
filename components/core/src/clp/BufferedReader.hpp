#ifndef CLP_BUFFEREDREADER_HPP
#define CLP_BUFFEREDREADER_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "BufferReader.hpp"
#include "ErrorCode.hpp"
#include "ReaderInterface.hpp"
#include "TraceableException.hpp"

namespace clp {
/**
 * Class for performing buffered (in memory) reads from another `ReaderInterface` (referred as
 * source reader) with control over when and how much data is buffered. This allows us to support
 * use cases where we want to perform unordered reads from inputs which have expensive random access
 * (e.g. files from object storage like S3).
 *
 * To control how much data is buffered, we allow callers to set a checkpoint such that all reads
 * and seeks past the checkpoint will be buffered until the checkpoint is cleared. This allows
 * callers to perform random seeks and reads of any data after (and including) the checkpoint.
 * When no checkpoint is set, we maintain a fixed-size buffer.
 *
 * NOTE: This class restricts the buffer size to a multiple of the page size and we avoid reading
 * anything less than a page to avoid multiple page faults.
 */
class BufferedReader : public ReaderInterface {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : OperationFailed(
                          error_code,
                          filename,
                          line_number,
                          "`BufferedReader` operation failed"
                  ) {}

        OperationFailed(
                ErrorCode error_code,
                char const* const filename,
                int line_number,
                std::string message
        )
                : TraceableException(error_code, filename, line_number),
                  m_message(std::move(message)) {}

        // Methods
        [[nodiscard]] auto what() const noexcept -> char const* override {
            return m_message.c_str();
        }

    private:
        std::string m_message;
    };

    // Constants
    static constexpr size_t cMinBufferSize = (1ULL << 12);

    // Constructors
    /**
     * @param reader_interface
     * @param base_buffer_size The size for the fixed-size buffer used when no checkpoint is set. It
     * must be a multiple of `BufferedReader::cMinBufferSize`.
     */
    explicit BufferedReader(
            std::shared_ptr<ReaderInterface> reader_interface,
            size_t base_buffer_size
    );

    explicit BufferedReader(std::shared_ptr<ReaderInterface> reader_interface)
            : BufferedReader{std::move(reader_interface), cDefaultBufferSize} {}

    // Destructor
    ~BufferedReader() override = default;

    // Delete copy & move constructors and assignment operators
    BufferedReader(BufferedReader const&) = delete;
    BufferedReader(BufferedReader&&) = delete;
    auto operator=(BufferedReader const&) -> BufferedReader& = delete;
    auto operator=(BufferedReader&&) -> BufferedReader& = delete;

    // Methods
    /**
     * Tries to fill the internal buffer if it's empty.
     * @return ErrorCode_Success on success.
     * @return Forwards `refill_reader_buffer`'s return values on failure.
     */
    [[nodiscard]] auto try_refill_buffer_if_empty() -> ErrorCode;

    /**
     * Peeks the remaining buffered content without advancing the read head.
     *
     * NOTE: Any subsequent read or seek operations may invalidate the returned buffer.
     * @param buf Returns a pointer to the remaining content in the buffer.
     * @param peek_size Returns the size of the remaining content in the buffer.
     */
    auto peek_buffered_data(char const*& buf, size_t& peek_size) const -> void;

    /**
     * Sets a checkpoint at the current position in the source reader. If a checkpoint is already
     * set, this method will discard any buffered content from before the current checkpoint.
     *
     * NOTE: Setting a checkpoint may result in higher memory usage since the `BufferedReader` needs
     * to buffer all the data it reads after the checkpoint.
     * @return The current position in the source reader.
     */
    [[maybe_unused]] auto set_checkpoint() -> size_t;

    /**
     * Clears the current checkpoint and moves the read head to the highest position that the caller
     * read/seeked to. This will shrink the buffer to its original size, discarding any excess data.
     */
    auto clear_checkpoint() -> void;

    // Methods implementing the ReaderInterface
    /**
     * @param pos Returns the position of the read head in the source reader.
     * @return ErrorCode_Success on success.
     */
    [[nodiscard]] auto try_get_pos(size_t& pos) -> ErrorCode override;

    /**
     * Tries to seek to the given position relative to the beginning of the source reader. When no
     * checkpoint is set, callers can only seek forwards; When a checkpoint is set, callers can
     * seek to any position in the source reader that's after and including the checkpoint.
     * @param pos
     * @return ErrorCode_Success on success.
     * @return ErrorCode_Unsupported if a checkpoint is set and the requested position is less than
     * the checkpoint, or no checkpoint is set and the requested position is less the current read
     * head's position.
     * @return ErrorCode_Truncated if we reached the EOF before reaching the given position.
     * @return Forwards `BufferReader::try_seek_from_begin`'s return values on failure.
     * @return Forwards `ReaderInterface::try_seek_from_begin`'s return values on failure.
     * @return Forwards `refill_reader_buffer`'s return values on failure.
     */
    [[nodiscard]] auto try_seek_from_begin(size_t pos) -> ErrorCode override;

    /**
     * Tries to read up to a given number of bytes from the current read head.
     * @param buf
     * @param num_bytes_to_read The number of bytes to try and read.
     * @param num_bytes_read Returns the actual number of bytes read.
     * @return ErrorCode_Success on success.
     * @return ErrorCode_BadParam if `buf` is nullptr.
     * @return ErrorCode_EndOfFile on EOF.
     * @return Forwards `BufferReader::try_read`'s return values on failure.
     * @return Forwards `refill_reader_buffer`'s return values on failure.
     */
    [[nodiscard]] auto try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read)
            -> ErrorCode override;

    /**
     * Tries to read up to an occurrence of the given delimiter.
     * @param delim
     * @param keep_delimiter Whether to include the delimiter in the output string.
     * @param append Whether to append to the given string or replace its contents.
     * @param str Returns the content read.
     * @return ErrorCode_Success on success.
     * @return ErrorCode_EndOfFile on EOF.
     * @return Forwards `BufferReader::try_read_to_delimiter`'s return values on failure.
     * @return Forwards `refill_reader_buffer`'s return values on failure.
     */
    [[nodiscard]] auto
    try_read_to_delimiter(char delim, bool keep_delimiter, bool append, std::string& str)
            -> ErrorCode override;

private:
    // Methods
    /**
     * Refills the buffer with up to the given number of bytes from the source reader.
     *
     * NOTE: Callers must ensure the current buffer has been exhausted before calling this method
     * (i.e., the read head is at the end of the buffer).
     * @param num_bytes_to_refill
     * @return Forwards `ReaderInterface::try_read`'s return values.
     */
    [[nodiscard]] auto refill_reader_buffer(size_t num_bytes_to_refill) -> ErrorCode;

    /**
     * Discards the data before the current position and resizes the buffer accordingly.
     */
    auto drop_content_before_current_pos() -> void;

    /**
     * @param in_src_pos
     * @return The position relative to the beginning of the buffer.
     */
    [[nodiscard]] auto get_in_buffer_pos(size_t in_src_pos) const -> size_t {
        return in_src_pos - m_buffer_begin_in_src_pos;
    }

    [[nodiscard]] auto get_buffer_end_in_src_pos() const -> size_t {
        return m_buffer_begin_in_src_pos + m_buffer_reader.get_buffer_size();
    }

    /**
     * Advances `m_in_src_pos` to `pos`; if `pos` >= `m_highest_read_in_src_pos`,
     * `m_highest_read_in_src_pos` is updated as well.
     * @param pos
     */
    auto update_in_src_pos(size_t pos) -> void;

    // Constants
    static constexpr size_t cDefaultBufferSize = (16 * cMinBufferSize);

    // Variables
    size_t m_in_src_pos{0};
    std::shared_ptr<ReaderInterface> m_reader;

    // Buffer specific data
    std::vector<char> m_buffer;
    size_t m_base_buffer_size{};
    BufferReader m_buffer_reader;
    size_t m_buffer_begin_in_src_pos{0};

    // Variables for checkpoint support
    std::optional<size_t> m_checkpoint_pos;
    size_t m_highest_read_in_src_pos{0};
};
}  // namespace clp

#endif  // CLP_BUFFEREDREADER_HPP
