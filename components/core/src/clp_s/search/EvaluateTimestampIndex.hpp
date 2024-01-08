#ifndef CLP_S_SEARCH_EVALUATETIMESTAMPINDEX_HPP
#define CLP_S_SEARCH_EVALUATETIMESTAMPINDEX_HPP

#include "../TimestampDictionaryReader.hpp"
#include "../Utils.hpp"
#include "Expression.hpp"

namespace clp_s::search {
class EvaluateTimestampIndex {
public:
    // Constructors
    EvaluateTimestampIndex(std::shared_ptr<TimestampDictionaryReader> const& timestamp_dict)
            : m_timestamp_dict(timestamp_dict) {}

    /**
     * Takes an expression and attempts to prove its output (true/false/unknown) based on
     * a timestamp index. Currently doesn't do any constant propagation.
     *
     * Should only be run after type narrowing.
     *
     * @param expr the expression to evaluate against the timestamp index
     * @return The evaluated value of the expression given the index (True, False, Unknown)
     */
    EvaluatedValue run(std::shared_ptr<Expression> const& expr);

private:
    std::shared_ptr<TimestampDictionaryReader> m_timestamp_dict;
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_EVALUATETIMESTAMPINDEX_HPP
