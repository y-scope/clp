#include "LogSurgeonReader.hpp"

namespace clp {
LogSurgeonReader::LogSurgeonReader(ReaderInterface& reader_interface)
        : m_reader_interface(reader_interface) {
    read = [this](char* buf, size_t count, size_t& read_to) -> log_surgeon::ErrorCode {
        m_reader_interface.read(buf, count, read_to);
        if (read_to == 0) {
            return log_surgeon::ErrorCode::EndOfFile;
        }
        return log_surgeon::ErrorCode::Success;
    };
}
}  // namespace clp
