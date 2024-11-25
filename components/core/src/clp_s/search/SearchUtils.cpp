#include "SearchUtils.hpp"

#include <cmath>

namespace clp_s::search {
void splice_into(
        std::shared_ptr<Expression> const& parent,
        std::shared_ptr<Expression> const& child,
        OpList::iterator location
) {
    for (auto it = child->op_begin(); it != child->op_end(); it++) {
        auto sub_expr = std::static_pointer_cast<Expression>(*it);
        sub_expr->set_parent(parent.get());
    }
    parent->get_op_list().splice(location, child->get_op_list());
}

// TODO: make sure to handle Object types correctly
LiteralType node_to_literal_type(NodeType type) {
    switch (type) {
        case NodeType::Integer:
            return LiteralType::IntegerT;
        case NodeType::Float:
            return LiteralType::FloatT;
        case NodeType::ClpString:
            return LiteralType::ClpStringT;
        case NodeType::VarString:
            return LiteralType::VarStringT;
        case NodeType::Boolean:
            return LiteralType::BooleanT;
        case NodeType::UnstructuredArray:
            return LiteralType::ArrayT;
        case NodeType::NullValue:
            return LiteralType::NullT;
        case NodeType::DateString:
            return LiteralType::EpochDateT;
        case NodeType::Metadata:
        case NodeType::Unknown:
        default:
            return LiteralType::UnknownT;
    }
}

bool double_as_int(double in, FilterOperation op, int64_t& out) {
    switch (op) {
        case FilterOperation::EQ:
            out = static_cast<int64_t>(in);
            return in == static_cast<double>(out);
        case FilterOperation::LT:
        case FilterOperation::GTE:
            out = std::ceil(in);
        case FilterOperation::GT:
        case FilterOperation::LTE:
            out = std::floor(in);
        default:
            out = static_cast<int64_t>(in);
    }
    return true;
}

bool wildcard_match(std::string_view s, std::string_view p) {
    size_t i, j, star, last;
    i = j = 0;
    star = last = -1;

    while (i < s.length()) {
        if (j < p.length() && (s[i] == p[j] || p[j] == '?')) {
            ++i;
            ++j;
        } else if (j < p.length() && p[j] == '*') {
            star = j++;
            last = i;
        } else if (star != -1) {
            i = last++;
            j = star + 1;
        } else {
            return false;
        }
    }

    while (j < p.length() && p[j] == '*') {
        ++j;
    }

    return j == p.length();
}
}  // namespace clp_s::search
