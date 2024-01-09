// Code from CLP

#ifndef CLP_S_FILEWRITER_HPP
#define CLP_S_FILEWRITER_HPP

#include <cstdio>
#include <string>

#include "ErrorCode.hpp"
#include "TraceableException.hpp"

namespace clp_s {
class FileWriter {
public:
    // Types
    enum class OpenMode {
        CreateForWriting,
        CreateIfNonexistentForAppending,
        CreateIfNonexistentForSeekableWriting,
    };

    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructors
    FileWriter() : m_file(nullptr), m_fd(-1) {}

    // Destructor
    ~FileWriter();

    // Methods implementing the WriterInterface
    /**
     * Writes a buffer to the file
     * @param data
     * @param data_length Length of the buffer
     * @throw FileWriter::OperationFailed on failure
     */
    void write(char const* data, size_t data_length);

    /**
     * Writes a numeric value to the file
     * @param val
     * @tparam ValueType
     */
    template <typename ValueType>
    void write_numeric_value(ValueType val) {
        write(reinterpret_cast<char*>(&val), sizeof(val));
    }

    /**
     * Flushes the file
     * @throw FileWriter::OperationFailed on failure
     */
    void flush();

    /**
     * Gets the current position of the write head in the file
     * @return Position of the write head in the file
     * @throw FileWriter::OperationFailed on failure
     */
    size_t get_pos();

    /**
     * Tries to get the current position of the write head in the file
     * @param pos Position of the write head in the file
     * @return ErrorCodeNotInit if the file is not open
     * @return ErrorCodeErrno on error
     * @return ErrorCodeSuccess on success
     */
    ErrorCode try_get_pos(size_t& pos) const;

    /**
     * Seeks from the beginning of the file to the given position
     * @param pos The position to seek to
     * @throw FileWriter::OperationFailed on failure
     */
    void seek_from_begin(size_t pos);

    /**
     * Tries to seek from the beginning of the file to the given position
     * @param pos
     * @return ErrorCodeNotInit if the file is not open
     * @return ErrorCodeErrno on error
     * @return ErrorCodeSuccess on success
     */
    ErrorCode try_seek_from_begin(size_t pos);

    /**
     * Tries to offset from the current position by the given amount
     * @param pos
     * @return ErrorCodeNotInit if the file is not open
     * @return ErrorCodeErrno on error
     * @return ErrorCodeSuccess on success
     */
    ErrorCode try_seek_from_current(off_t offset);

    // Methods
    /**
     * Opens a file for writing
     * @param path
     * @param open_mode The mode to open the file with
     * @throw FileWriter::OperationFailed on failure
     */
    void open(std::string const& path, OpenMode open_mode);

    /**
     * Closes the file
     * @throw FileWriter::OperationFailed on failure
     */
    void close();

private:
    FILE* m_file;
    int m_fd;
};
}  // namespace clp_s

#endif  // CLP_S_FILEWRITER_HPP
