#ifndef CLP_S_SEARCH_EVALUATE_RANGE_INDEX_FILTERS_HPP
#define CLP_S_SEARCH_EVALUATE_RANGE_INDEX_FILTERS_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "../ArchiveReaderAdaptor.hpp"
#include "../Utils.hpp"
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
 *
 * The pass also records the `log_event_idx` ranges that the whole expression could match, so that
 * records outside of them can be skipped without being evaluated.
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
     * @return The sorted [begin, end) `log_event_idx` ranges that records matching the last
     * expression passed to `run` must fall within, or an empty vector if the expression contains no
     * filters in the "$" namespace.
     */
    [[nodiscard]] auto get_log_event_idx_ranges() const
            -> std::vector<std::pair<size_t, size_t>> const& {
        return m_log_event_idx_ranges;
    }

private:
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

    /**
     * Evaluates an expression against a JSON object, treating filters outside of the "$" namespace
     * as unknown.
     * @param expr
     * @param fields
     * @return The result of evaluating `expr` against the `fields` JSON object.
     */
    auto evaluate_expression(ast::Expression* expr, nlohmann::json const& fields) const
            -> EvaluatedValue;

    std::vector<clp_s::RangeIndexEntry> const& m_range_index;
    bool m_case_sensitive_match{false};
    std::vector<std::pair<size_t, size_t>> m_log_event_idx_ranges;
};
}  // namespace clp_s::search
#endif  // CLP_S_SEARCH_EVALUATE_RANGE_INDEX_FILTERS_HPP
