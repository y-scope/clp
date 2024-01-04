#include "ReaderInterface.hpp"

using std::string;

namespace clp {
ErrorCode ReaderInterface::try_read_to_delimiter(
        char delim,
        bool keep_delimiter,
        bool append,
        std::string& str
) {
    if (false == append) {
        str.clear();
    }

    size_t original_str_length = str.length();

    // Read character by character into str, until we find a delimiter
    char c;
    size_t num_bytes_read;
    while (true) {
        auto error_code = try_read(&c, 1, num_bytes_read);
        if (ErrorCode_Success != error_code) {
            if (ErrorCode_EndOfFile == error_code && str.length() > original_str_length) {
                return ErrorCode_Success;
            }
            return error_code;
        }

        if (delim == c) {
            break;
        }

        str += c;
    }

    // Add delimiter if necessary
    if (keep_delimiter) {
        str += delim;
    }

    return ErrorCode_Success;
}

bool ReaderInterface::read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) {
    ErrorCode error_code = try_read(buf, num_bytes_to_read, num_bytes_read);
    if (ErrorCode_EndOfFile == error_code) {
        return false;
    }
    if (ErrorCode_Success != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
    return true;
}

bool ReaderInterface::read_to_delimiter(char delim, bool keep_delimiter, bool append, string& str) {
    ErrorCode error_code = try_read_to_delimiter(delim, keep_delimiter, append, str);
    if (ErrorCode_EndOfFile == error_code) {
        return false;
    }
    if (ErrorCode_Success != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }

    return true;
}

ErrorCode ReaderInterface::try_read_exact_length(char* buf, size_t num_bytes) {
    size_t num_bytes_read;
    auto error_code = try_read(buf, num_bytes, num_bytes_read);
    if (ErrorCode_Success != error_code) {
        return error_code;
    }
    if (num_bytes_read < num_bytes) {
        return ErrorCode_Truncated;
    }

    return ErrorCode_Success;
}

bool ReaderInterface::read_exact_length(char* buf, size_t num_bytes, bool eof_possible) {
    ErrorCode error_code = try_read_exact_length(buf, num_bytes);
    if (eof_possible && ErrorCode_EndOfFile == error_code) {
        return false;
    }
    if (ErrorCode_Success != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
    return true;
}

ErrorCode ReaderInterface::try_read_string(size_t const str_length, string& str) {
    // Resize string to fit str_length
    str.resize(str_length);

    return try_read_exact_length(&str[0], str_length);
}

bool ReaderInterface::read_string(size_t const str_length, string& str, bool eof_possible) {
    ErrorCode error_code = try_read_string(str_length, str);
    if (eof_possible && ErrorCode_EndOfFile == error_code) {
        return false;
    }
    if (ErrorCode_Success != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
    return true;
}

void ReaderInterface::seek_from_begin(size_t pos) {
    ErrorCode error_code = try_seek_from_begin(pos);
    if (ErrorCode_Success != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
}

size_t ReaderInterface::get_pos() {
    size_t pos;
    ErrorCode error_code = try_get_pos(pos);
    if (ErrorCode_Success != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }

    return pos;
}
}  // namespace clp
