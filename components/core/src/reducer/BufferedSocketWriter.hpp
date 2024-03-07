#ifndef REDUCER_BUFFERED_SOCKET_WRITER_HPP
#define REDUCER_BUFFERED_SOCKET_WRITER_HPP

#include <cstdint>
#include <type_traits>
#include <vector>

namespace reducer {
/**
 * A buffered writer to a network socket.
 */
class BufferedSocketWriter {
public:
    BufferedSocketWriter(int socket_fd, size_t buffer_size) : m_socket_fd{socket_fd} {
        m_buffer.reserve(buffer_size);
    }

    /**
     * Appends data to an internal buffer and writes the contents of the buffer to the network
     * socket whenever the buffer becomes full.
     * @param data the data being written
     * @return whether the write was successful or not
     */
    template <
            typename T,
            typename = std::enable_if_t<
                    std::is_pod<T>::value && std::is_trivially_copyable<T>::value
                    && sizeof(T) == sizeof(char)>>
    bool write(std::vector<T> const& data) {
        return write(reinterpret_cast<char const*>(data.data()), data.size());
    }

    /**
     * Appends data to an internal buffer and writes the contents of the buffer to the network
     * socket whenever the buffer becomes full.
     * @param data the data being written
     * @param size the size of the data being written in bytes
     * @return whether the write was successful or not
     */
    bool write(char const* data, size_t size);

    /**
     * Flushes the contents of the interal buffer to the network socket.
     * @return whether the flush was successful or not
     */
    bool flush();

private:
    int m_socket_fd;
    std::vector<uint8_t> m_buffer;
};
}  // namespace reducer
#endif  // REDUCER_BUFFERED_SOCKET_WRITER_HPP
