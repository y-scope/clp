#ifndef CLP_SYSFILEREADER_HPP
#define CLP_SYSFILEREADER_HPP

#include <sys/stat.h>

#include <string_view>

#include "ErrorCode.hpp"
#include "FileDescriptor.hpp"
#include "ReaderInterface.hpp"
#include "TraceableException.hpp"

namespace clp {
class SysFileReader : public ReaderInterface {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override { return "FileReader operation failed"; }
    };

    SysFileReader(std::string_view const& path)
            : m_fd{path, FileDescriptor::OpenMode::ReadOnly},
              m_path{path} {}

    // Explicitly disable copy constructor and assignment operator
    SysFileReader(SysFileReader const&) = delete;
    SysFileReader& operator=(SysFileReader const&) = delete;

    // Move constructor and assignment operator
    SysFileReader(SysFileReader&&);
    SysFileReader& operator=(SysFileReader&&) = delete;

    // Methods implementing the ReaderInterface
    /**
     * Tries to get the current position of the read head in the file
     * @param pos Position of the read head in the file
     * @return ErrorCode_errno on error
     * @return ErrorCode_Success on success
     */
    [[nodiscard]] auto try_get_pos(size_t& pos) -> ErrorCode override;
    /**
     * Tries to seek from the beginning of the file to the given position
     * @param pos
     * @return ErrorCode_errno on error
     * @return ErrorCode_Success on success
     */
    [[nodiscard]] auto try_seek_from_begin(size_t pos) -> ErrorCode override;

    /**
     * Tries to read up to a given number of bytes from the file
     * @param buf
     * @param num_bytes_to_read The number of bytes to try and read
     * @param num_bytes_read The actual number of bytes read
     * @return ErrorCode_BadParam if buf is invalid
     * @return ErrorCode_errno on error
     * @return ErrorCode_EndOfFile on EOF
     * @return ErrorCode_Success on success
     */
    [[nodiscard]] auto
    try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) -> ErrorCode override;

    [[nodiscard]] auto get_path() const -> std::string_view { return m_path; }

    /**
     * Tries to stat the current file
     * @param stat_buffer
     * @return ErrorCode_errno on error
     * @return ErrorCode_Success on success
     */
    [[nodiscard]] auto try_fstat(struct stat& stat_buffer) const -> ErrorCode;

private:
    FileDescriptor m_fd;
    std::string m_path;
};
}  // namespace clp

#endif  // CLP_SYSFILEREADER_HPP
