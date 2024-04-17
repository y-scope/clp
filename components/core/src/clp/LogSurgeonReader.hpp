#ifndef CLP_LOGSURGEONREADER_HPP
#define CLP_LOGSURGEONREADER_HPP

#include <log_surgeon/Reader.hpp>

#include "ReaderInterface.hpp"

namespace clp {
/*
 * Wrapper providing a read function that works with the parsers in log_surgeon.
 */
class LogSurgeonReader : public log_surgeon::Reader {
public:
    LogSurgeonReader(ReaderInterface& reader_interface);

private:
    ReaderInterface& m_reader_interface;
};
}  // namespace clp

#endif  // CLP_LOGSURGEONREADER_HPP
