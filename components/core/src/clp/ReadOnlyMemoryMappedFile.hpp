#ifndef CLP_READONLYMEMORYMAPPEDFILE_HPP
#define CLP_READONLYMEMORYMAPPEDFILE_HPP

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace clp {
/**
 * A class for mapping a read-only file into memory. It maintains the memory buffer created by the
 * underlying `mmap` system call and provides methods to get a view of the memory buffer.
 */
class ReadOnlyMemoryMappedFile {
public:
    // Constructors
    /**
     * @param path The path of the file to map.
     */
    explicit ReadOnlyMemoryMappedFile(std::string_view path) noexcept;

    // Destructor
    ~ReadOnlyMemoryMappedFile();

    // Disable copy/move constructors/assignment operators
    ReadOnlyMemoryMappedFile(ReadOnlyMemoryMappedFile const&) = delete;
    ReadOnlyMemoryMappedFile(ReadOnlyMemoryMappedFile&&) = delete;
    auto operator=(ReadOnlyMemoryMappedFile const&) -> ReadOnlyMemoryMappedFile& = delete;
    auto operator=(ReadOnlyMemoryMappedFile&&) -> ReadOnlyMemoryMappedFile& = delete;

    /**
     * @return The errno captured from the last system call failure. A value of 0 indicates that no
     * error occurred.
     */
    [[nodiscard]] auto get_errno() const -> int { return m_errno; }

    /**
     * @return A view of the mapped file in memory, or an empty span if the file is not mapped.
     */
    [[nodiscard]] auto get_view() const -> std::span<char> {
        if (false == is_open()) {
            return {};
        }
        return std::span<char>{static_cast<char*>(m_data), m_buf_size};
    }

    /**
     * @return Whether the file is currently memory mapped and viewable.
     */
    [[nodiscard]] auto is_open() const -> bool {
        return m_data != nullptr && m_buf_size != 0 && 0 == m_errno;
    }

private:
    void* m_data{nullptr};
    size_t m_buf_size{0};
    int m_errno{0};
};
}  // namespace clp

#endif
