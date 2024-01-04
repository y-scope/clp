#ifndef CLP_WRITERINTERFACE_HPP
#define CLP_WRITERINTERFACE_HPP

#include <cstddef>
#include <string>

#include "ErrorCode.hpp"
#include "TraceableException.hpp"

namespace clp {
class WriterInterface {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override { return "WriterInterface operation failed"; }
    };

    // Methods
    /**
     * Writes the given data to the underlying medium
     * @param data
     * @param data_length
     */
    virtual void write(char const* data, size_t data_length) = 0;
    virtual void flush() = 0;
    virtual ErrorCode try_seek_from_begin(size_t pos) = 0;
    virtual ErrorCode try_seek_from_current(off_t offset) = 0;
    virtual ErrorCode try_get_pos(size_t& pos) const = 0;

    /**
     * Writes a numeric value
     * @param val Value to write
     */
    template <typename ValueType>
    void write_numeric_value(ValueType value);

    /**
     * Writes a character to the underlying medium
     * @param c
     */
    void write_char(char c);
    /**
     * Writes a string to the underlying medium
     * @param str
     */
    void write_string(std::string const& str);

    /**
     * Seeks from the beginning to the given position
     * @param pos
     */
    void seek_from_begin(size_t pos);

    /**
     * Offsets from the current position by the given amount
     * @param offset
     */
    void seek_from_current(off_t offset);

    /**
     * Gets the current position of the write head
     * @return Position of the write head
     */
    size_t get_pos() const;
};

template <typename ValueType>
void WriterInterface::write_numeric_value(ValueType val) {
    write(reinterpret_cast<char*>(&val), sizeof(val));
}
}  // namespace clp

#endif  // CLP_WRITERINTERFACE_HPP
