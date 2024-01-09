#include "StringLiteral.hpp"

#include <sstream>

#include "SearchUtils.hpp"

namespace clp_s::search {
std::shared_ptr<Literal> StringLiteral::create(std::string const& v) {
    return std::shared_ptr<Literal>(static_cast<Literal*>(new StringLiteral(v)));
}

void StringLiteral::print() {
    get_print_stream() << "\"" << m_v << "\"";
}

std::string& StringLiteral::get() {
    return m_v;
}

bool StringLiteral::as_clp_string(std::string& ret, FilterOperation op) {
    if (op == FilterOperation::LT || op == FilterOperation::GT || op == FilterOperation::LTE
        || op == FilterOperation::GTE)
    {
        return false;
    }

    if (false == matches_type(LiteralType::ClpStringT)) {
        return false;
    }

    ret = m_v;
    return true;
}

bool StringLiteral::as_var_string(std::string& ret, FilterOperation op) {
    if (op == FilterOperation::LT || op == FilterOperation::GT || op == FilterOperation::LTE
        || op == FilterOperation::GTE)
    {
        return false;
    }

    if (false == matches_type(LiteralType::VarStringT)) {
        return false;
    }

    ret = m_v;
    return true;
}

bool StringLiteral::as_float(double& ret, FilterOperation op) {
    std::istringstream ss(m_v);
    ss >> std::noskipws >> ret;
    return !ss.fail() && ss.eof();
}

bool StringLiteral::as_int(int64_t& ret, FilterOperation op) {
    std::istringstream ss(m_v);
    ss >> std::noskipws >> ret;
    if (false == ss.fail() && ss.eof()) {
        return true;
    } else {
        double tmp;
        ss = std::istringstream(m_v);
        ss >> std::noskipws >> tmp;
        if (false == ss.fail() && ss.eof()) {
            return double_as_int(tmp, op, ret);
        }
    }
    return false;
}

bool StringLiteral::as_bool(bool& ret, FilterOperation op) {
    if (op == FilterOperation::LT || op == FilterOperation::GT || op == FilterOperation::LTE
        || op == FilterOperation::GTE)
    {
        return false;
    }
    if (m_v == "true") {
        ret = true;
        return true;
    } else if (m_v == "false") {
        ret = false;
        return true;
    }
    return false;
}

bool StringLiteral::as_null(FilterOperation op) {
    return (op == FilterOperation::EQ || op == FilterOperation::NEQ) && m_v == "null";
}

bool StringLiteral::as_any(FilterOperation op) {
    return (op == FilterOperation::EQ || op == FilterOperation::NEQ) && m_v == "*";
}
}  // namespace clp_s::search
