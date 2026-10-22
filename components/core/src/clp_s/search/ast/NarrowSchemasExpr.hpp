#ifndef CLP_S_SEARCH_AST_NARROWSCHEMASEXPR_HPP
#define CLP_S_SEARCH_AST_NARROWSCHEMASEXPR_HPP

#include <cstdint>
#include <memory>
#include <unordered_set>

#include <clp_s/search/ast/Expression.hpp>

namespace clp_s::search::ast {
/**
 * An expression with no sub-expressions that holds for an event iff the event's schema is one of
 * `schema_ids`, or, when inverted, iff it is not. It constrains no column value.
 *
 * Only `SchemaMatch` creates this expression, to narrow the schemas an enclosing expression can
 * match to a set it has already computed (e.g. the schemas whose log shape satisfies a query).
 * `SchemaMatch` resolves it while splitting the query by schema, so no per-schema query contains
 * one.
 */
class NarrowSchemasExpr : public Expression {
public:
    // Factory methods
    /**
     * @param schema_ids The schemas the expression holds for.
     * @param parent The parent this expression is attached to, if any.
     * @return The newly created expression.
     */
    [[nodiscard]] static auto
    create(std::unordered_set<int32_t> schema_ids, Expression* parent = nullptr)
            -> std::shared_ptr<Expression>;

    // Delete copy assignment operator
    auto operator=(NarrowSchemasExpr const&) -> NarrowSchemasExpr& = delete;

    // Delete move constructor and assignment operator
    NarrowSchemasExpr(NarrowSchemasExpr&&) = delete;
    auto operator=(NarrowSchemasExpr&&) -> NarrowSchemasExpr& = delete;

    // Destructor
    ~NarrowSchemasExpr() override = default;

    // Methods implementing Expression
    [[nodiscard]] auto has_only_expression_operands() -> bool override { return false; }

    [[nodiscard]] auto copy() const -> std::shared_ptr<Expression> override;

    // Methods implementing Value
    auto print() const -> void override;

    // Methods
    /**
     * @return The schemas the expression holds for (or, when inverted, does not hold for).
     */
    [[nodiscard]] auto get_schema_ids() const -> std::unordered_set<int32_t> const& {
        return m_schema_ids;
    }

private:
    // Constructors
    explicit NarrowSchemasExpr(std::unordered_set<int32_t> schema_ids, Expression* parent);

    NarrowSchemasExpr(NarrowSchemasExpr const&) = default;

    // Data members
    std::unordered_set<int32_t> m_schema_ids;
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_AST_NARROWSCHEMASEXPR_HPP
