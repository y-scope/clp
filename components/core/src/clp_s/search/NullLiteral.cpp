#include "NullLiteral.hpp"

namespace clp_s::search {
std::shared_ptr<Literal> NullLiteral::create() {
    return std::shared_ptr<Literal>(new NullLiteral());
}

std::shared_ptr<Literal> NullLiteral::create_from_string(std::string const& v) {
    if (v == "null") {
        return std::shared_ptr<Literal>(new NullLiteral());
    }

    return {nullptr};
}

void NullLiteral::print() {
    get_print_stream() << "null";
}

bool NullLiteral::as_var_string(std::string& ret, FilterOperation op) {
    if (op == FilterOperation::EQ || op == FilterOperation::NEQ) {
        ret = "null";
        return true;
    }

    return false;
}

bool NullLiteral::as_null(FilterOperation op) {
    return op == FilterOperation::EQ || op == FilterOperation::NEQ;
}
}  // namespace clp_s::search
