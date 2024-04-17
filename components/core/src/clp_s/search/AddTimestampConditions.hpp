#ifndef CLP_S_SEARCH_ADDTIMESTAMPCONDITIONS_HPP
#define CLP_S_SEARCH_ADDTIMESTAMPCONDITIONS_HPP

#include <optional>
#include <string>
#include <vector>

#include "../Defs.hpp"
#include "Transformation.hpp"

namespace clp_s::search {
/**
 * Transformation pass which takes optional begin and end filters on a timestamp column and augments
 * the AST with additional filters on the provided timestamp column.
 *
 * Providing an empty option for begin_ts and end_ts always results in no change to the AST.
 *
 * If either begin_ts or end_ts is specified and the timestamp_column option is empty then this
 * transformation pass will yield EmptyExpr.
 */
class AddTimestampConditions : public Transformation {
public:
    // Constructors
    AddTimestampConditions(
            std::optional<std::vector<std::string>> const& timestamp_column,
            std::optional<epochtime_t> begin_ts,
            std::optional<epochtime_t> end_ts
    )
            : m_timestamp_column(timestamp_column),
              m_begin_ts(begin_ts),
              m_end_ts(end_ts) {}

    /**
     * Takes in an AST and adds filters on the provided timestamp column.
     * @param expr the AST to transform
     * @return the transformed AST
     */
    std::shared_ptr<Expression> run(std::shared_ptr<Expression>& expr) override;

private:
    std::optional<std::vector<std::string>> m_timestamp_column;
    std::optional<epochtime_t> m_begin_ts;
    std::optional<epochtime_t> m_end_ts;
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_ADDTIMESTAMPCONDITIONS_HPP
