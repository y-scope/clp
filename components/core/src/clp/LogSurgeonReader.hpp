#ifndef CLP_LOGSURGEONREADER_HPP
#define CLP_LOGSURGEONREADER_HPP

#include <io_interface/ReaderInterface.hpp>
#include <log_surgeon/Reader.hpp>

namespace clp {
/*
 * Wrapper providing a read function that works with the parsers in log_surgeon.
 */
class LogSurgeonReader : public log_surgeon::Reader {
public:
    LogSurgeonReader(::clp::io_interface::ReaderInterface& reader_interface);

private:
    ::clp::io_interface::ReaderInterface& m_reader_interface;
};
}  // namespace clp

#endif  // CLP_LOGSURGEONREADER_HPP
