#include "MemoryMappedFileView.hpp"

#include <sys/mman.h>

#include <string>
#include <string_view>

#include "ErrorCode.hpp"
#include "FileDescriptor.hpp"

namespace clp {
MemoryMappedFileView::MemoryMappedFileView(std::string_view path) {
    FileDescriptor const fd{path, FileDescriptor::Mode::ReadOnly};
    auto const file_size{fd.get_size()};
    auto* mmap_ptr{mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd.get_raw_fd(), 0)};
    if (MAP_FAILED == mmap_ptr) {
        throw OperationFailed(
                ErrorCode_errno,
                __FILE__,
                __LINE__,
                "`mmap` failed to map the file in path: " + std::string{path}
        );
    }
    m_data = mmap_ptr;
    m_buf_size = file_size;
}

MemoryMappedFileView::~MemoryMappedFileView() {
    if (0 == m_buf_size) {
        // If the mapped region has a length of 0, we should not call `munmap` since 0 is an invalid
        // length to unmap a region.
        return;
    }
    munmap(m_data, m_buf_size);
}
}  // namespace clp
