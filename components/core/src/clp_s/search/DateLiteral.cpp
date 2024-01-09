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
    std::istringstream ss(v);
    epochtime_t tmp_int_epoch;
    double tmp_double_epoch;

    ss >> std::noskipws >> tmp_int_epoch;
    if (false == ss.fail() && ss.eof()) {
        return std::shared_ptr<Literal>(static_cast<Literal*>(new DateLiteral(tmp_int_epoch, v)));
    }

    ss = std::istringstream(v);
    ss >> std::noskipws >> tmp_double_epoch;
    if (false == ss.fail() && ss.eof()) {
        return std::shared_ptr<Literal>(static_cast<Literal*>(new DateLiteral(tmp_double_epoch, v))
        );
    }

    // begin end arguments are returned only -- their value doesn't matter
    size_t timestamp_begin_pos = 0, timestamp_end_pos = 0;
    auto pattern = TimestampPattern::search_known_ts_patterns(
            v,
            tmp_int_epoch,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    if (pattern == nullptr) {
        return std::shared_ptr<Literal>(nullptr);
    }

    return std::shared_ptr<Literal>(static_cast<Literal*>(new DateLiteral(tmp_int_epoch, v)));
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
