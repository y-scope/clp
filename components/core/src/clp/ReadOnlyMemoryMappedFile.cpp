#include "ReadOnlyMemoryMappedFile.hpp"

#include <sys/mman.h>

#include <cerrno>
#include <string_view>

#include <spdlog/spdlog.h>

#include "FileDescriptor.hpp"
#include "TraceableException.hpp"

namespace clp {
ReadOnlyMemoryMappedFile::ReadOnlyMemoryMappedFile(std::string_view path) noexcept {
    try {
        FileDescriptor const fd{path, FileDescriptor::OpenMode::ReadOnly};
        auto const file_size{fd.get_size()};
        if (0 == file_size) {
            // `mmap` doesn't allow mapping an empty file, so we don't need to call it.
            return;
        }

        // TODO: optionally provide a hint for the mmap location
        auto* mmap_ptr{mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd.get_raw_fd(), 0)};
        if (MAP_FAILED == mmap_ptr) {
            m_errno = errno;
            return;
        }

        m_data = mmap_ptr;
        m_buf_size = file_size;
    } catch (TraceableException const& ex) {
        m_errno = errno;
        SPDLOG_ERROR(
                "ReadOnlyMemoryMappedFile: File descriptor error with path: {}. Error: {}",
                path.data(),
                ex.what()
        );
    }
}

ReadOnlyMemoryMappedFile::~ReadOnlyMemoryMappedFile() {
    if (false == is_open()) {
        return;
    }
    // We skip error checking since the only likely reason for `munmap` to fail is if we give it
    // invalid arguments.
    munmap(m_data, m_buf_size);
}
}  // namespace clp
