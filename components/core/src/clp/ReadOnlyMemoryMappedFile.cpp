#include "ReadOnlyMemoryMappedFile.hpp"

#include <sys/mman.h>

#include <string>
#include <string_view>

#include "ErrorCode.hpp"
#include "FileDescriptor.hpp"

namespace clp {
ReadOnlyMemoryMappedFile::ReadOnlyMemoryMappedFile(std::string_view path) {
    FileDescriptor const fd{path, FileDescriptor::OpenMode::ReadOnly};
    auto const file_size{fd.get_size()};
    if (0 == file_size) {
        // `mmap` doesn't allow mapping an empty file, so we don't need to call it.
        return;
    }
    auto* mmap_ptr{mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd.get_raw_fd(), 0)};
    if (MAP_FAILED == mmap_ptr) {
        throw OperationFailed(
                ErrorCode_errno,
                __FILE__,
                __LINE__,
                "`mmap` failed to map path: " + std::string{path}
        );
    }
    m_data = mmap_ptr;
    m_buf_size = file_size;
}

ReadOnlyMemoryMappedFile::~ReadOnlyMemoryMappedFile() {
    if (0 == m_buf_size) {
        // We don't call `mmap` for empty files, so we don't need to call `munmap`.
        return;
    }
    // We skip error checking since the only likely reason for `munmap` to fail is if we give it
    // invalid arguments.
    munmap(m_data, m_buf_size);
}
}  // namespace clp
