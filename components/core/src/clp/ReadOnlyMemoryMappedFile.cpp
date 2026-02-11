#include "ReadOnlyMemoryMappedFile.hpp"

#include <sys/mman.h>

#include <cerrno>
#include <string_view>
#include <system_error>

#include <ystdlib/error_handling/Result.hpp>

#include "FileDescriptor.hpp"
#include "TraceableException.hpp"

namespace clp {
auto ReadOnlyMemoryMappedFile::create(std::string_view path)
        -> ystdlib::error_handling::Result<ReadOnlyMemoryMappedFile> {
    try {
        FileDescriptor const fd{path, FileDescriptor::OpenMode::ReadOnly};
        auto const file_size{fd.get_size()};
        if (0 == file_size) {
            // `mmap` doesn't allow mapping an empty file, so we don't need to call it.
            return ReadOnlyMemoryMappedFile{nullptr, file_size};
        }

        auto* mmap_ptr{mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd.get_raw_fd(), 0)};
        if (MAP_FAILED == mmap_ptr) {
            return static_cast<std::errc>(errno);
        }

        return ReadOnlyMemoryMappedFile{mmap_ptr, file_size};
    } catch (TraceableException const&) {
        // TODO: Rewrite `FileDescriptor` constructor and `get_size()`
        // with `ystdlib::error_handling::result` for capturing errno at the point of failure and
        // better error propagation.
        // Issue: #1971
        return static_cast<std::errc>(errno);
    }
}

ReadOnlyMemoryMappedFile::~ReadOnlyMemoryMappedFile() {
    if (nullptr == m_data) {
        return;
    }
    // We skip error checking since the only likely reason for `munmap` to fail is if we give it
    // invalid arguments.
    munmap(m_data, m_buf_size);
}
}  // namespace clp
