#include "DateLiteral.hpp"

#include <sstream>

#include "../../Defs.hpp"
#include "SearchUtils.hpp"

namespace clp_s::search::ast {
namespace {
constexpr epochtime_t cNanosecondsInMicrosecond{1000LL};
constexpr epochtime_t cNanosecondsInMillisecond{1000LL * cNanosecondsInMicrosecond};
constexpr epochtime_t cNanosecondsInSecond{1000LL * cNanosecondsInMillisecond};
}  // namespace

DateLiteral::DateLiteral(epochtime_t v)
        : m_timestamp{v},
          m_default_precision_timestamp{v},
          m_double_timestamp{static_cast<double>(v) / cNanosecondsInSecond} {}

std::shared_ptr<Literal> DateLiteral::create(epochtime_t v) {
    return std::shared_ptr<Literal>(static_cast<Literal*>(new DateLiteral(v)));
}

void DateLiteral::print() const {
    get_print_stream() << "timestamp(" << m_default_precision_timestamp << ")";
}

bool DateLiteral::as_int(int64_t& ret, FilterOperation op) {
    ret = m_default_precision_timestamp;
    return true;
}

bool DateLiteral::as_float(double& ret, FilterOperation op) {
    ret = m_double_timestamp;
    return true;
}

auto DateLiteral::as_precision(Precision precision) -> epochtime_t {
    switch (precision) {
        case Precision::Seconds:
            return m_timestamp / cNanosecondsInSecond;
        case Precision::Milliseconds:
            return m_timestamp / cNanosecondsInMillisecond;
        case Precision::Microseconds:
            return m_timestamp / cNanosecondsInMicrosecond;
        case Precision::Nanoseconds:
            return m_timestamp;
        default:
            return m_timestamp;
    }
}

void DateLiteral::set_default_precision(Precision precision) {
    m_default_precision_timestamp = as_precision(precision);
}
}  // namespace clp_s::search::ast
