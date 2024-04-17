#include "Integral.hpp"

#include <sstream>

#include "SearchUtils.hpp"

namespace clp_s::search {
Integral::Integral(double v) : m_v(v) {}

Integral::Integral(int64_t v) : m_v(v) {}

std::shared_ptr<Literal> Integral::create_from_float(double v) {
    return std::shared_ptr<Literal>(static_cast<Literal*>(new Integral(v)));
}

std::shared_ptr<Literal> Integral::create_from_int(int64_t v) {
    return std::shared_ptr<Literal>(static_cast<Literal*>(new Integral(v)));
}

std::shared_ptr<Literal> Integral::create_from_string(std::string const& v) {
    Integral* ret = nullptr;
    int64_t tmpint;
    std::istringstream ss(v);
    ss >> std::noskipws >> tmpint;
    if (false == ss.fail() && ss.eof()) {
        ret = new Integral(tmpint);
        ret->m_vstr = v;
        return std::shared_ptr<Literal>(static_cast<Literal*>(ret));
    }

    double tmpdouble;
    ss = std::istringstream(v);
    ss >> std::noskipws >> tmpdouble;
    if (false == ss.fail() && ss.eof()) {
        ret = new Integral(tmpdouble);
        ret->m_vstr = v;
        return std::shared_ptr<Literal>(static_cast<Literal*>(ret));
    }
    return std::shared_ptr<Literal>(static_cast<Literal*>(ret));
}

void Integral::print() {
    auto& os = get_print_stream();
    if (false == m_vstr.empty()) {
        os << m_vstr;
    } else if (std::holds_alternative<int64_t>(m_v)) {
        os << std::get<int64_t>(m_v);
    } else {
        os << std::get<double>(m_v);
    }
}

Integral64 Integral::get() {
    return m_v;
}

bool Integral::as_var_string(std::string& ret, FilterOperation op) {
    if (op == FilterOperation::LT || op == FilterOperation::GT || op == FilterOperation::LTE
        || op == FilterOperation::GTE)
    {
        return false;
    }
    if (false == m_vstr.empty()) {
        ret = m_vstr;
    } else {
        std::ostringstream ss;
        if (std::holds_alternative<double>(m_v)) {
            ss << std::get<double>(m_v);
        } else {
            ss << std::get<int64_t>(m_v);
        }
        m_vstr = ss.str();
        ret = m_vstr;
    }
    return true;
}

bool Integral::as_float(double& ret, FilterOperation op) {
    if (std::holds_alternative<double>(m_v)) {
        ret = std::get<double>(m_v);
    } else {
        ret = std::get<int64_t>(m_v);
    }
    return true;
}

bool Integral::as_int(int64_t& ret, FilterOperation op) {
    if (std::holds_alternative<double>(m_v)) {
        double tmp = std::get<double>(m_v);
        return double_as_int(tmp, op, ret);
    } else {
        ret = std::get<int64_t>(m_v);
    }
    return true;
}
}  // namespace clp_s::search
