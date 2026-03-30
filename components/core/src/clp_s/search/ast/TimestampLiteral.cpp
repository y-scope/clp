#include "TimestampLiteral.hpp"

#include <sstream>

#include "../../Defs.hpp"
#include "SearchUtils.hpp"

namespace clp_s::search::ast {
namespace {
constexpr epochtime_t cNanosecondsInMicrosecond{1000LL};
constexpr epochtime_t cNanosecondsInMillisecond{1000LL * cNanosecondsInMicrosecond};
constexpr epochtime_t cNanosecondsInSecond{1000LL * cNanosecondsInMillisecond};
}  // namespace

TimestampLiteral::TimestampLiteral(epochtime_t v)
        : m_timestamp{v},
          m_default_precision_timestamp{v},
          m_double_timestamp{static_cast<double>(v) / cNanosecondsInSecond} {}

std::shared_ptr<Literal> TimestampLiteral::create(epochtime_t v) {
    return std::shared_ptr<Literal>(new TimestampLiteral(v));
}

void TimestampLiteral::print() const {
    get_print_stream() << "timestamp(" << m_default_precision_timestamp << ")";
}

bool TimestampLiteral::as_int(int64_t& ret, FilterOperation op) {
    ret = m_default_precision_timestamp;
    return true;
}

bool TimestampLiteral::as_float(double& ret, FilterOperation op) {
    ret = m_double_timestamp;
    return true;
}

auto TimestampLiteral::as_precision(Precision precision) const -> epochtime_t {
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

void TimestampLiteral::set_default_precision(Precision precision) {
    m_default_precision_timestamp = as_precision(precision);
}
}  // namespace clp_s::search::ast
