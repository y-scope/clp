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

        // TODO: optionally provide a hint for the mmap location
        auto* mmap_ptr{mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd.get_raw_fd(), 0)};
        if (MAP_FAILED == mmap_ptr) {
            return std::error_code(errno, std::system_category());
        }

        return ReadOnlyMemoryMappedFile{mmap_ptr, file_size};
    } catch (TraceableException const& ex) {
        // TODO: Rewrite `FileDescriptor` constructor and `get_size()` to capture errno at the point
        // of failure
        return std::error_code(errno, std::system_category());
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
