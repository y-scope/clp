#ifndef REDUCER_BUFFEREDSOCKETWRITER_HPP
#define REDUCER_BUFFEREDSOCKETWRITER_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace reducer {
/**
 * A buffered writer to a network socket.
 */
class BufferedSocketWriter {
public:
    BufferedSocketWriter(int socket_fd, size_t buffer_size)
            : m_socket_fd{socket_fd},
              m_buffer_capacity{buffer_size} {
        m_buffer.reserve(buffer_size);
    }

    /**
     * Appends data to an internal buffer and writes the contents of the buffer to the network
     * socket whenever the buffer becomes full.
     * @param data The data to write
     * @return Whether the write was successful
     */
    template <
            typename T,
            typename = std::enable_if_t<
                    std::is_trivial_v<T> && std::is_standard_layout_v<T>
                    && std::is_trivially_copyable_v<T> && sizeof(T) == sizeof(char)>>
    bool write(std::vector<T> const& data) {
        return write(reinterpret_cast<char const*>(data.data()), data.size());
    }

    /**
     * Appends data to an internal buffer and writes the contents of the buffer to the network
     * socket whenever the buffer becomes full.
     * @param data The data to write
     * @param size The size, in bytes, of the data to write
     * @return Whether the write was successful
     */
    bool write(char const* data, size_t size);

    /**
     * Flushes the contents of the internal buffer to the network socket.
     * @return Whether the flush was successful
     */
    bool flush();

private:
    /**
     * Flushes the contents of the internal buffer to the network socket, but without checking if
     * the buffer has data to flush.
     * @return Whether the flush was successful
     */
    bool flush_unsafe();

    int m_socket_fd;
    std::vector<uint8_t> m_buffer;
    size_t m_buffer_capacity;
};
}  // namespace reducer
#endif  // REDUCER_BUFFEREDSOCKETWRITER_HPP
