#ifndef CLP_S_ADD_TIMESTAMP_CONDITIONS_HPP
#define CLP_S_ADD_TIMESTAMP_CONDITIONS_HPP

#include <optional>

#include "../Defs.hpp"
#include "../TimestampDictionaryReader.hpp"
#include "Transformation.hpp"

namespace clp_s::search {
class AddTimestampConditions : public Transformation {
public:
    // Constructors
    AddTimestampConditions(
            std::shared_ptr<TimestampDictionaryReader> const& timestamp_dict,
            std::optional<epochtime_t> begin_ts,
            std::optional<epochtime_t> end_ts
    )
            : m_timestamp_dict(timestamp_dict),
              m_begin_ts(begin_ts),
              m_end_ts(end_ts) {}

    /**
     * Takes in an AST and adds filters on the authoratative timestamp column if the user specified
     * such filters on the command line.
     */
    std::shared_ptr<Expression> run(std::shared_ptr<Expression>& expr) override;

private:
    std::shared_ptr<TimestampDictionaryReader> m_timestamp_dict;
    std::optional<epochtime_t> m_begin_ts;
    std::optional<epochtime_t> m_end_ts;
};
}  // namespace clp_s::search

#endif  // CLP_S_ADD_TIMESTAMP_CONDITIONS_HPP
