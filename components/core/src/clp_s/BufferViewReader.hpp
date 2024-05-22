#ifndef CLP_S_BUFFER_VIEW_READER_HPP
#define CLP_S_BUFFER_VIEW_READER_HPP

#include "TraceableException.hpp"
#include "Utils.hpp"

namespace clp_s {
/**
 * BufferViewReader is a utility class designed to let several readers safely share views
 * into a shared buffer. Readers can consume from the buffer either by consuming spans and scalar
 * values from the buffer. Any attempt to consume past the end of the buffer will throw.
 * @throws OperationFailed
 */
class BufferViewReader {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    explicit BufferViewReader(char* buffer, size_t size)
            : m_buffer(buffer),
              m_remaining_size(size) {}

    template <typename T>
    T read_value() {
        if (m_remaining_size < sizeof(T)) {
            throw OperationFailed(ErrorCodeOutOfBounds, __FILENAME__, __LINE__);
        }
        T tmp;
        memcpy(&tmp, m_buffer, sizeof(T));
        m_buffer += sizeof(T);
        m_remaining_size -= sizeof(T);
        return tmp;
    }

    template <typename T>
    UnalignedMemSpan<T> read_unaligned_span(size_t length) {
        size_t span_length_in_bytes = sizeof(T) * length;
        if (m_remaining_size < span_length_in_bytes) {
            throw OperationFailed(ErrorCodeOutOfBounds, __FILENAME__, __LINE__);
        }
        UnalignedMemSpan<T> tmp{m_buffer, length};
        m_buffer += span_length_in_bytes;
        m_remaining_size -= span_length_in_bytes;
        return tmp;
    }

    size_t get_remaining_size() { return m_remaining_size; }

private:
    char* m_buffer{};
    size_t m_remaining_size{};
};
}  // namespace clp_s
#endif  // CLP_S_BUFFER_VIEW_READER_HPP
