#include "NarrowSchemasExpr.hpp"

#include <cstdint>
#include <memory>
#include <ostream>
#include <unordered_set>
#include <utility>

#include <clp_s/search/ast/Expression.hpp>

namespace clp_s::search::ast {
NarrowSchemasExpr::NarrowSchemasExpr(std::unordered_set<int32_t> schema_ids, Expression* parent)
        : Expression{false, parent},
          m_schema_ids{std::move(schema_ids)} {}

auto NarrowSchemasExpr::create(std::unordered_set<int32_t> schema_ids, Expression* parent)
        -> std::shared_ptr<Expression> {
    return std::shared_ptr<Expression>{new NarrowSchemasExpr{std::move(schema_ids), parent}};
}

auto NarrowSchemasExpr::print() const -> void {
    auto& os{get_print_stream()};
    os << (is_inverted() ? "!" : "") << "NarrowSchemasExpr(" << m_schema_ids.size() << " schemas)";
    if (nullptr == get_parent()) {
        os << '\n';
    }
    os << std::flush;
}

auto NarrowSchemasExpr::copy() const -> std::shared_ptr<Expression> {
    return std::shared_ptr<Expression>{new NarrowSchemasExpr{*this}};
}
}  // namespace clp_s::search::ast
