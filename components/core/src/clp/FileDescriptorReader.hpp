#ifndef CLP_FILEDESCRIPTORREADER_HPP
#define CLP_FILEDESCRIPTORREADER_HPP

#include <sys/stat.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include "ErrorCode.hpp"
#include "FileDescriptor.hpp"
#include "ReaderInterface.hpp"
#include "TraceableException.hpp"

namespace clp {
/**
 * Class for performing direct reads from an on-disk file using `clp::FileDescriptor` and C-style
 * system call. Unlike `clp::FileReader`, which uses on `FILE` stream interface to buffer read data,
 * this class does not buffer data internally. Instead, the user of this class is expected to
 * buffer and read the data efficiently.
 *
 * Note: If you don't plan to handle the data buffering yourself, do not use this class. Use
 * `clp::FileReader` instead.
 */
class FileDescriptorReader : public ReaderInterface {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        [[nodiscard]] auto what() const noexcept -> char const* override {
            return "clp::FileDescriptorReader operation failed";
        }
    };

    // Constructors
    explicit FileDescriptorReader(std::string path)
            : m_path{std::move(path)},
              m_fd{m_path, FileDescriptor::OpenMode::ReadOnly} {}

    // Explicitly disable copy constructor and assignment operator
    FileDescriptorReader(FileDescriptorReader const&) = delete;
    auto operator=(FileDescriptorReader const&) -> FileDescriptorReader& = delete;

    // Explicitly disable move constructor and assignment operator
    FileDescriptorReader(FileDescriptorReader&&) = delete;
    auto operator=(FileDescriptorReader&&) -> FileDescriptorReader& = delete;

    // Destructor
    ~FileDescriptorReader() override = default;

    // Methods implementing the ReaderInterface
    /**
     * Tries to read up to a given number of bytes from the file.
     * @param buf
     * @param num_bytes_to_read The number of bytes to try and read
     * @param num_bytes_read The actual number of bytes read
     * @return ErrorCode_BadParam if buf is invalid
     * @return ErrorCode_errno on error
     * @return ErrorCode_EndOfFile on EOF
     * @return ErrorCode_Success on success
     */
    [[nodiscard]] auto try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read)
            -> ErrorCode override;

    /**
     * Tries to seek to the given position, relative to the beginning of the file.
     * @param pos
     * @return ErrorCode_errno on error
     * @return ErrorCode_Success on success
     */
    [[nodiscard]] auto try_seek_from_begin(size_t pos) -> ErrorCode override;

    /**
     @param pos Returns the position of the read head in the buffer.
     * @return ErrorCode_errno on error
     * @return ErrorCode_Success on success
     */
    [[nodiscard]] auto try_get_pos(size_t& pos) -> ErrorCode override;

    // Methods
    [[nodiscard]] auto get_path() const -> std::string_view { return m_path; }

    /**
     * Obtains information about the open file associated with the underlying file descriptor.
     * @param stat_buffer Returns the stat results.
     * @return Same as `FileDescriptor::fstat`
     */
    [[nodiscard]] auto try_fstat(struct stat& stat_buffer) const -> ErrorCode {
        return m_fd.stat(stat_buffer);
    }

private:
    std::string m_path;
    FileDescriptor m_fd;
};
}  // namespace clp

#endif  // CLP_FILEDESCRIPTORREADER_HPP
