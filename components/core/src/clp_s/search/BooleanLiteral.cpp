#include "BooleanLiteral.hpp"

namespace clp_s::search {
std::shared_ptr<Literal> BooleanLiteral::create_from_bool(bool v) {
    return std::shared_ptr<Literal>(new BooleanLiteral(v));
}

std::shared_ptr<Literal> BooleanLiteral::create_from_string(std::string const& v) {
    if (v == "true") {
        return std::shared_ptr<Literal>(new BooleanLiteral(true));
    } else if (v == "false") {
        return std::shared_ptr<Literal>(new BooleanLiteral(false));
    }

    return {nullptr};
}

void BooleanLiteral::print() {
    auto& os = get_print_stream();
    if (m_v) {
        os << "true";
    } else {
        os << "false";
    }
}

bool BooleanLiteral::as_var_string(std::string& ret, FilterOperation op) {
    if (op == FilterOperation::EQ || op == FilterOperation::NEQ) {
        ret = m_v ? "true" : "false";
        return true;
    }

    return false;
}

bool BooleanLiteral::as_bool(bool& ret, FilterOperation op) {
    if (op == FilterOperation::EQ || op == FilterOperation::NEQ) {
        ret = m_v;
        return true;
    }

    return false;
}
}  // namespace clp_s::search
