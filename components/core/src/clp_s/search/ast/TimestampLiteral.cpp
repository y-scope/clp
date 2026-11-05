#include "TimestampLiteral.hpp"

#include <cstdint>
#include <memory>

#include <clp_s/Defs.hpp>

#include "FilterOperation.hpp"
#include "Literal.hpp"

namespace clp_s::search::ast {
namespace {
constexpr epochtime_t cNanosecondsInMicrosecond{1000LL};
constexpr epochtime_t cNanosecondsInMillisecond{1000LL * cNanosecondsInMicrosecond};
constexpr epochtime_t cNanosecondsInSecond{1000LL * cNanosecondsInMillisecond};
}  // namespace

TimestampLiteral::TimestampLiteral(epochtime_t timestamp)
        : m_timestamp{timestamp},
          m_default_precision_timestamp{timestamp},
          m_double_timestamp{static_cast<double>(timestamp) / cNanosecondsInSecond} {}

auto TimestampLiteral::create(epochtime_t v) -> std::shared_ptr<Literal> {
    return std::shared_ptr<Literal>(new TimestampLiteral(v));
}

auto TimestampLiteral::print() const -> void {
    get_print_stream() << "timestamp(" << m_default_precision_timestamp << ")";
}

auto TimestampLiteral::as_int(int64_t& ret, FilterOperation /*op*/) -> bool {
    ret = m_default_precision_timestamp;
    return true;
}

auto TimestampLiteral::as_float(double& ret, FilterOperation /*op*/) -> bool {
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
        default:
            return m_timestamp;
    }
}

auto TimestampLiteral::set_default_precision(Precision precision) -> void {
    m_default_precision_timestamp = as_precision(precision);
}
}  // namespace clp_s::search::ast
