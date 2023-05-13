#include "FileReader.hpp"

// Boost libraries
#include <boost/filesystem.hpp>

// C standard libraries
// C libraries
#include <sys/stat.h>
#include <fcntl.h>

// C++ standard libraries
#include <cerrno>

// Project headers
#include <spdlog/spdlog.h>

using std::string;

FileReader::~FileReader () {
    close();
    free(m_read_buffer);
}

ErrorCode FileReader::try_get_pos (size_t& pos) {
    if (-1 == m_fd) {
        return ErrorCode_NotInit;
    }

    pos = m_file_pos;
    return ErrorCode_Success;
}

static ErrorCode try_read_into_buffer(int fd, int8_t* buffer, size_t num_bytes_to_read,
                                      size_t& num_bytes_read) {
    num_bytes_read = 0;
    while (true) {
        auto bytes_read = ::read(fd, buffer + num_bytes_read, num_bytes_to_read);
        if (bytes_read == -1) {
            return ErrorCode_errno;
        }
        if (bytes_read == 0) {
            break;
        }
        num_bytes_read += bytes_read;
        num_bytes_to_read -= bytes_read;
        if (num_bytes_to_read == 0) {
            return ErrorCode_Success;
        }
    }
    if (num_bytes_read == 0) {
        return ErrorCode_EndOfFile;
    }
    return ErrorCode_Success;
}


ErrorCode FileReader::refill_reader_buffer (size_t num_bytes_to_read) {
    size_t num_bytes_read {0};
    if (false == m_checkpoint_enabled) {
        auto error_code = try_read_into_buffer(m_fd, m_read_buffer,
                                               num_bytes_to_read, num_bytes_read);
        if (error_code != ErrorCode_Success) {
            return error_code;
        }
        reset_buffer(m_read_buffer, num_bytes_read);
    } else {
        // increase buffer size
        m_read_buffer = (int8_t*)realloc(m_read_buffer, m_size + num_bytes_to_read);
        m_buffer = m_read_buffer;
        auto error_code = try_read_into_buffer(m_fd, m_read_buffer + m_size, num_bytes_to_read,
                                               num_bytes_read);
        if (error_code != ErrorCode_Success) {
            return error_code;
        }
        m_size += num_bytes_read;
    }
    return ErrorCode_Success;
}

ErrorCode FileReader::try_read (char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) {
    if (-1 == m_fd) {
        return ErrorCode_NotInit;
    }
    if (nullptr == buf) {
        return ErrorCode_BadParam;
    }

    num_bytes_read = 0;
    size_t num_bytes_to_read_from_buffer {num_bytes_to_read};
    size_t num_bytes_read_from_buffer {0};
    // keep reading
    while (true) {
        auto error_code = BufferReader::try_read(buf + num_bytes_read,
                                                 num_bytes_to_read_from_buffer,
                                                 num_bytes_read_from_buffer);
        if (ErrorCode_Success == error_code ||
            ErrorCode_EndOfFile == error_code ||
            ErrorCode_NotInit == error_code)
        {
            m_file_pos += num_bytes_read_from_buffer;
            num_bytes_read += num_bytes_read_from_buffer;
            num_bytes_to_read_from_buffer -= num_bytes_read_from_buffer;
            if (num_bytes_to_read_from_buffer == 0) {
                break;
            } else {
                // else, we refill the buffer
                error_code = refill_reader_buffer(cReaderBufferSize);
                // TODO: here if refill_reader_buffer returns eof, we can't simply
                // return eof, because we might have already readed some data
                if (ErrorCode_EndOfFile == error_code) {
                    if (num_bytes_read == 0) {
                        return ErrorCode_EndOfFile;
                    } else {
                        break;
                    }
                }
                else if (ErrorCode_Success != error_code) {
                    return error_code;
                }
            }
        } else {
            return error_code;
        }
    }
    return ErrorCode_Success;
}

// Maybe everytime, I should always read a page?
ErrorCode FileReader::try_seek_from_begin (size_t pos) {
    if (m_fd == -1) {
        return ErrorCode_NotInit;
    }
    // early return path
    if (pos == m_file_pos) {
        return ErrorCode_Success;
    }

    if (pos > m_file_pos) {
        auto front_seek_amount = pos - m_file_pos;
        if (front_seek_amount > m_size - m_cursor_pos) {
            if (m_checkpoint_enabled == false) {
                // let's assume we want the read to be always page aligned
                auto buffer_aligned_pos = pos & cBufferAlignedMask;
                auto offset = lseek(m_fd, buffer_aligned_pos, SEEK_SET);
                if (offset == -1) {
                    return ErrorCode_errno;
                }
                // now the issue is that: if we want to delay buffer loading
                // how can we propogate the right m_cursor_pos
                if (auto error_code = refill_reader_buffer(cReaderBufferSize);
                    ErrorCode_Success != error_code) {
                    return error_code;
                }
                m_file_pos = pos;
                m_cursor_pos = pos - buffer_aligned_pos;
            } else {
                // Get the file size
                struct stat fileInfo;
                fstat(m_fd, &fileInfo);
                off_t file_size = fileInfo.st_size;
                if (pos > file_size) {
                    SPDLOG_ERROR("not expecting to seek pass the Entire file");
                    throw;
                }

                size_t data_read_remaining = front_seek_amount;
                // we want to load all file contents between into the buffer
                // TODO: what if without reading the files, user seek to a further place
                // hence the buffer is nullptr pointing. and then user set the checkpoint,
                // and then seek again? in this case, it should still work? because the refill
                // buffer will handle it interanlly
                while (true) {
                    // keep refilling the buffer
                    if (auto error_code = refill_reader_buffer(cReaderBufferSize);
                            ErrorCode_Success != error_code) {
                        return error_code;
                    }
                    if (data_read_remaining < cReaderBufferSize) {
                        m_file_pos = pos;
                        m_cursor_pos += front_seek_amount;
                        // then we are done.
                        break;
                    }
                    data_read_remaining -= cReaderBufferSize;
                }
            }
        } else {
            // otherwise, we can simply seek in the same buffer;
            m_cursor_pos += front_seek_amount;
            m_file_pos = pos;
        }
    } else {
        if (false == m_checkpoint_enabled) {
            SPDLOG_ERROR("Seek back not allowed when checkpoint is not enabled");
            return ErrorCode_Failure;
        }
        if (pos < m_checkpointed_pos) {
            SPDLOG_ERROR("Seek back before the checkpoint is not supported");
            return ErrorCode_Failure;
        }
        m_file_pos = pos;
        m_cursor_pos = m_checkpointed_buffer_pos + (pos - m_checkpointed_pos);
    }
    return ErrorCode_Success;
}


ErrorCode FileReader::try_open (const string& path) {
    // Cleanup in case caller forgot to call close before calling this function
    close();

    m_fd = ::open(path.c_str(), O_RDONLY);
    if (-1 == m_fd) {
        if (ENOENT == errno) {
            return ErrorCode_FileNotFound;
        }
        return ErrorCode_errno;
    }
    m_path = path;
    m_file_pos = 0;

    // Buffer specific things
    reset_buffer(nullptr, 0);
    return ErrorCode_Success;
}

ErrorCode FileReader::try_read_to_delimiter (char delim, bool keep_delimiter,
                                             bool append, string& str) {
    assert(-1 != m_fd);

    if (false == append) {
        str.clear();
    }

    bool found_delim {false};

    while (false == found_delim) {
        auto cursor {m_cursor_pos};
        while (cursor < m_size && false == found_delim) {
            if (delim == m_buffer[cursor]) {
                found_delim = true;
            }
            cursor++;
        }
        // append to strings
        std::string_view substr(reinterpret_cast<const char*>(m_buffer + m_cursor_pos),
                                cursor - m_cursor_pos);
        str.append(substr);
        // increase file pos
        m_file_pos += cursor - m_cursor_pos;
        m_cursor_pos = cursor;
        if (false == found_delim) {
            if (auto error_code = refill_reader_buffer(cReaderBufferSize);
                    ErrorCode_Success != error_code) {
                return error_code;
            }
        }
    }
    return ErrorCode_Success;
}

void FileReader::open (const string& path) {
    ErrorCode error_code = try_open(path);
    if (ErrorCode_Success != error_code) {
        if (ErrorCode_FileNotFound == error_code) {
            throw "File not found: " + boost::filesystem::weakly_canonical(path).string() + "\n";
        } else {
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }
    }
}

void FileReader::revert_pos() {
    if (false == m_checkpoint_enabled) {
        SPDLOG_ERROR("Checkpoint is not enabled");
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_file_pos = m_checkpointed_pos;
    // this should have revert the pos to the original buffer pos
    m_cursor_pos = m_checkpointed_buffer_pos;
}

void FileReader::mark_pos() {
    if (true == m_checkpoint_enabled) {
        SPDLOG_ERROR("I haven't carefully think about whether we should allow this or not");
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_checkpointed_pos = m_file_pos;
    m_checkpoint_enabled = true;
    m_checkpointed_buffer_pos = m_cursor_pos;
}

// let's assume the checkpoint can only be reset if we are already reading
// recent data
void FileReader::reset_checkpoint () {

    // we don't allow to reset yet
    // because currently we are still reading from buffered data
    // and we are not at the last "page" of data
    if (m_size - m_cursor_pos > cReaderBufferSize) {
        SPDLOG_ERROR("Not ready for reset checkpoint");
        // or we can only remove out of dated buffer, and reclain memory when
        // we read the next page
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    if(m_size > cReaderBufferSize) {
        auto new_buffer = (int8_t*)malloc(sizeof(int8_t) * cReaderBufferSize);
        // copy the last "page" of data over.
        size_t copy_pos = m_size - cReaderBufferSize;
        memcpy(new_buffer, m_buffer + copy_pos, cReaderBufferSize);
        free(m_read_buffer);
        m_read_buffer = new_buffer;

        m_size = cReaderBufferSize;
        m_buffer = new_buffer;
        m_cursor_pos -= copy_pos;
    }
    m_checkpoint_enabled = false;
}

void FileReader::close () {
    if (-1 != m_fd) {
        // NOTE: We don't check errors for fclose since it seems
        // the only reason it could fail is if it was interrupted by a signal
        ::close(m_fd);
        m_fd = -1;

        if (m_checkpoint_enabled) {
            // TODO: add a debug log message
            m_read_buffer = (int8_t*)realloc(m_read_buffer, cReaderBufferSize);
            m_checkpoint_enabled = false;
        }
    }
}

ErrorCode FileReader::try_fstat (struct stat& stat_buffer) const {
    if (-1 == m_fd) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    auto return_value = fstat(m_fd, &stat_buffer);
    if (0 != return_value) {
        return ErrorCode_errno;
    }
    return ErrorCode_Success;
}
