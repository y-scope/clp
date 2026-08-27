#ifndef CLP_S_SEARCH_EVALUATE_RANGE_INDEX_FILTERS_HPP
#define CLP_S_SEARCH_EVALUATE_RANGE_INDEX_FILTERS_HPP

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "../ArchiveReaderAdaptor.hpp"
#include "ast/Expression.hpp"
#include "ast/FilterExpr.hpp"
#include "ast/Transformation.hpp"
#include "nlohmann/json_fwd.hpp"

namespace clp_s::search {
/**
 * This transformation pass evaluates filters containing columns in the "$" namespace against the
 * metadata range index. Filters that match some range of the metadata index are rewritten into
 * filters against the "log_event_idx" column in the metadata subtree of the MPT. Filters that do
 * not match any part of the metadata range index are replaced with `EmptyExpr`.
 */
class EvaluateRangeIndexFilters : public ast::Transformation {
public:
    explicit EvaluateRangeIndexFilters(
            std::vector<clp_s::RangeIndexEntry> const& range_index,
            bool case_sensitive_match
    )
            : m_range_index{range_index},
              m_case_sensitive_match{case_sensitive_match} {}

    auto run(std::shared_ptr<ast::Expression>& expr) -> std::shared_ptr<ast::Expression> override;

    /**
     * @return The merged `log_event_idx` ranges a scan may restrict itself to, or an empty vector
     * if it must not skip anything. Non-empty only when every disjunct of the query is guarded by
     * a "$" filter, so that a row outside every range cannot satisfy the query by any other path.
     */
    [[nodiscard]] auto get_skippable_ranges() const -> std::vector<std::pair<size_t, size_t>> const& {
        return m_skippable_ranges;
    }

private:
    /**
     * Determines whether rows outside the matched ranges can be skipped, and collects those ranges.
     *
     * Must run before any filter is rewritten: a rewritten "$" filter is indistinguishable from an
     * ordinary `log_event_idx` filter, so a later check would not recognize its siblings as
     * guarded and would give up on queries it could have accelerated.
     * @param expr
     */
    void collect_skippable_ranges(ast::Expression* expr);

    /**
     * @param expr
     * @return Whether every disjunct reachable from `expr` contains a "$" filter. An inverted
     * expression anywhere makes this false: negation turns a guard into its opposite, and rows
     * outside the ranges may then satisfy the query.
     */
    [[nodiscard]] auto every_disjunct_is_guarded(ast::Expression* expr) const -> bool;

    /**
     * Collects the ranges matched by every "$" filter reachable from `expr` into
     * `m_skippable_ranges`.
     * @param expr
     */
    void gather_matching_ranges(ast::Expression* expr);
    /**
     * Evaluate a filter containing a column in the "$" namespace against the metadata range index
     * and re-write the filter accordingly.
     * @param filter_expr
     * @param parent_it Iterator in the parent expression containing `filter_expr`.
     * @param ast_root Reference to the root of the AST.
     */
    void evaluate_and_rewrite_filter(
            ast::FilterExpr* filter_expr,
            std::optional<ast::OpList::iterator> parent_it,
            std::shared_ptr<ast::Expression>& ast_root
    ) const;

    /**
     * Evaluates a filter against a JSON object.
     * @param filter_expr
     * @param fields
     * @return The result of evaluating `filter_expr` against the `fields` JSON object.
     */
    auto evaluate_filter(ast::FilterExpr* filter_expr, nlohmann::json const& fields) const -> bool;

    std::vector<clp_s::RangeIndexEntry> const& m_range_index;
    bool m_case_sensitive_match{false};
    std::vector<std::pair<size_t, size_t>> m_skippable_ranges;
};
}  // namespace clp_s::search
#endif  // CLP_S_SEARCH_EVALUATE_RANGE_INDEX_FILTERS_HPP
