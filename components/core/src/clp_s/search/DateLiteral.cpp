#include "DateLiteral.hpp"

#include <sstream>

#include "../TimestampPattern.hpp"
#include "SearchUtils.hpp"

namespace clp_s::search {
DateLiteral::DateLiteral(double v, std::string s) : Integral(v), m_epoch_str(std::move(s)) {}

DateLiteral::DateLiteral(epochtime_t v, std::string s) : Integral(v), m_epoch_str(std::move(s)) {}

std::shared_ptr<Literal> DateLiteral::create_from_float(double v) {
    std::ostringstream s;
    s << v;
    s.str();
    return std::shared_ptr<Literal>(static_cast<Literal*>(new DateLiteral(v, s.str())));
}

std::shared_ptr<Literal> DateLiteral::create_from_int(epochtime_t v) {
    std::ostringstream s;
    s << v;
    s.str();
    return std::shared_ptr<Literal>(static_cast<Literal*>(new DateLiteral(v, s.str())));
}

std::shared_ptr<Literal> DateLiteral::create_from_string(std::string const& v) {
    // begin end arguments are returned only -- their value doesn't matter
    size_t timestamp_begin_pos{0};
    size_t timestamp_end_pos{0};
    epochtime_t timestamp;
    auto pattern = TimestampPattern::search_known_ts_patterns(
            v,
            timestamp,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    if (pattern == nullptr) {
        return std::shared_ptr<Literal>(nullptr);
    }

    return std::shared_ptr<Literal>(static_cast<Literal*>(new DateLiteral(timestamp, v)));
}

void DateLiteral::print() {
    get_print_stream() << m_epoch_str;
}

bool DateLiteral::as_clp_string(std::string& ret, FilterOperation op) {
    if (op == FilterOperation::LT || op == FilterOperation::GT || op == FilterOperation::LTE
        || op == FilterOperation::GTE)
    {
        return false;
    }

    if (m_epoch_str.find(' ') == std::string::npos) {
        return false;
    }

    ret = m_epoch_str;
    return true;
}

bool DateLiteral::as_var_string(std::string& ret, FilterOperation op) {
    if (op == FilterOperation::LT || op == FilterOperation::GT || op == FilterOperation::LTE
        || op == FilterOperation::GTE)
    {
        return false;
    }

    if (m_epoch_str.find(' ') != std::string::npos) {
        return false;
    }

    ret = m_epoch_str;
    return true;
}
}  // namespace clp_s::search
