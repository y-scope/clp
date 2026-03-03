#ifndef CLP_READONLYMEMORYMAPPEDFILE_HPP
#define CLP_READONLYMEMORYMAPPEDFILE_HPP

#include <cstddef>
#include <span>
#include <string_view>
#include <utility>

#include <ystdlib/error_handling/Result.hpp>

namespace clp {
/**
 * A class for mapping a read-only file into memory. It maintains the memory buffer created by the
 * underlying `mmap` system call and provides methods to get a view of the memory buffer.
 */
class ReadOnlyMemoryMappedFile {
public:
    // Factory functions
    /**
     * @param path The path of the file to map.
     * @return A result containing the newly constructed `ReadOnlyMemoryMappedFile` on success, or
     * an error code indicating the failure:
     * - An instance of `std::errc` representing one of the following errors:
     *   - The `errno` value set by a failed `mmap` call. See also:
     *     https://man7.org/linux/man-pages/man2/mmap.2.html
     *   - The `errno` value set by failed `FileDescriptor` operations.
     */
    [[nodiscard]] static auto create(std::string_view path)
            -> ystdlib::error_handling::Result<ReadOnlyMemoryMappedFile>;

    // Destructor
    ~ReadOnlyMemoryMappedFile();

    // Delete copy constructor and assignment operator
    ReadOnlyMemoryMappedFile(ReadOnlyMemoryMappedFile const&) = delete;
    auto operator=(ReadOnlyMemoryMappedFile const&) -> ReadOnlyMemoryMappedFile& = delete;

    // Move constructor
    ReadOnlyMemoryMappedFile(ReadOnlyMemoryMappedFile&& rhs) noexcept
            : m_data{std::exchange(rhs.m_data, nullptr)},
              m_buf_size{std::exchange(rhs.m_buf_size, 0)} {}

    // Delete move assignment operator
    auto operator=(ReadOnlyMemoryMappedFile&& rhs) noexcept -> ReadOnlyMemoryMappedFile& = delete;

    /**
     * @return A view of the mapped file in memory, or an empty span if the file is not mapped.
     */
    [[nodiscard]] auto get_view() const -> std::span<char> {
        if (nullptr == m_data) {
            return {};
        }
        return std::span<char>{static_cast<char*>(m_data), m_buf_size};
    }

private:
    // Constructors
    ReadOnlyMemoryMappedFile(void* data, size_t buf_size) : m_data{data}, m_buf_size{buf_size} {}

    // Members
    void* m_data{nullptr};
    size_t m_buf_size{0};
};
}  // namespace clp

#endif
