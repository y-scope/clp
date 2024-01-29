#ifndef CLP_S_ADD_TIMESTAMP_CONDITIONS_HPP
#define CLP_S_ADD_TIMESTAMP_CONDITIONS_HPP

#include <optional>
#include <string>
#include <vector>

#include "../Defs.hpp"
#include "Transformation.hpp"

namespace clp_s::search {
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
     * Takes in an AST and adds filters on the authoritative timestamp column if the user specified
     * such filters on the command line.
     */
    std::shared_ptr<Expression> run(std::shared_ptr<Expression>& expr) override;

private:
    std::optional<std::vector<std::string>> m_timestamp_column;
    std::optional<epochtime_t> m_begin_ts;
    std::optional<epochtime_t> m_end_ts;
};
}  // namespace clp_s::search

#endif  // CLP_S_ADD_TIMESTAMP_CONDITIONS_HPP
