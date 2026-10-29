#ifndef CLP_S_SEARCH_TIMESTAMPLITERAL_HPP
#define CLP_S_SEARCH_TIMESTAMPLITERAL_HPP

#include <cstdint>
#include <memory>

#include "../../Defs.hpp"
#include "Literal.hpp"

namespace clp_s::search::ast {
/**
 * Class for timestamp literal in the search AST. Represents time
 * in epoch time.
 */
class TimestampLiteral : public Literal {
public:
    // Types
    enum class Precision : uint8_t {
        Seconds,
        Milliseconds,
        Microseconds,
        Nanoseconds
    };

    // Factory function
    /**
     * Creates a timestamp literal from a timestamp in epoch nanoseconds.
     * @param v The timestamp value.
     * @return A `shared_ptr` referencing the newly created timestamp literal.
     */
    static std::shared_ptr<Literal> create(epochtime_t v);

    // Delete copy constructor and assignment operator.
    TimestampLiteral(TimestampLiteral const&) = delete;
    TimestampLiteral& operator=(TimestampLiteral const&) = delete;

    // Methods inherited from Value
    void print() const override;

    // Methods inherited from Literal
    bool matches_type(LiteralType type) override { return type & cTimestampLiteralTypes; }

    bool matches_any(literal_type_bitmask_t mask) override { return mask & cTimestampLiteralTypes; }

    bool matches_exactly(literal_type_bitmask_t mask) override {
        return mask == cTimestampLiteralTypes;
    }

    bool as_timestamp() override { return true; }

    bool as_int(int64_t& ret, FilterOperation op) override;

    bool as_float(double& ret, FilterOperation op) override;

    // Methods
    /**
     * @param precision
     * @return The literal timestamp as the given precision (rounded towards zero).
     */
    [[nodiscard]] auto as_precision(Precision precision) const -> epochtime_t;

    /**
     * Sets the precision of the timestamp returned by `as_int`. The returned value from `as_int` is
     * the same as the value returned from `as_precision(precision)`.
     * @param precision
     */
    void set_default_precision(Precision precision);

private:
    // Types
    static constexpr literal_type_bitmask_t cTimestampLiteralTypes{TimestampT};

    // Constructors
    explicit TimestampLiteral(epochtime_t timestamp);

    // Variables
    epochtime_t m_timestamp{};
    epochtime_t m_default_precision_timestamp{};
    double m_double_timestamp{};
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_TIMESTAMPLITERAL_HPP
