#include "StringReader.hpp"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>

#include <boost/filesystem.hpp>

using std::string;

namespace glt {
StringReader::~StringReader() {
    close();
    free(m_getdelim_buf);
}

ErrorCode StringReader::try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) {
    if (input_string.empty()) {
        return ErrorCode_NotInit;
    }
    if (nullptr == buf) {
        return ErrorCode_BadParam;
    }

    if (pos == input_string.size()) {
        return ErrorCode_EndOfFile;
    }

    if (pos + num_bytes_to_read > input_string.size()) {
        num_bytes_to_read = input_string.size() - pos;
    }
    for (int i = 0; i < num_bytes_to_read; i++) {
        buf[i] = input_string[i + pos];
    }
    num_bytes_read = num_bytes_to_read;
    pos += num_bytes_read;
    return ErrorCode_Success;
}

ErrorCode StringReader::try_seek_from_begin(size_t pos) {
    this->pos = pos;
    return ErrorCode_Success;
}

ErrorCode StringReader::try_get_pos(size_t& pos) {
    pos = this->pos;
    return ErrorCode_Success;
}

ErrorCode StringReader::try_open(string const& input_string) {
    this->input_string = input_string;
    string_is_set = true;
    return ErrorCode_Success;
}

void StringReader::open(string const& input_string) {
    try_open(input_string);
}

void StringReader::close() {}
}  // namespace glt
