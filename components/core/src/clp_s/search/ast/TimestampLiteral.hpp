#ifndef CLP_S_SEARCH_TIMESTAMPLITERAL_HPP
#define CLP_S_SEARCH_TIMESTAMPLITERAL_HPP

#include <cstdint>
#include <memory>

#include <clp_s/Defs.hpp>

#include "FilterOperation.hpp"
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

    // Factory methods
    /**
     * Creates a timestamp literal from a timestamp in epoch nanoseconds.
     * @param v The timestamp value.
     * @return A `shared_ptr` referencing the newly created timestamp literal.
     */
    [[nodiscard]] static auto create(epochtime_t v) -> std::shared_ptr<Literal>;

    // Delete copy constructor and assignment operator
    TimestampLiteral(TimestampLiteral const&) = delete;
    auto operator=(TimestampLiteral const&) -> TimestampLiteral& = delete;

    // Delete move constructor and assignment operator
    TimestampLiteral(TimestampLiteral&&) = delete;
    auto operator=(TimestampLiteral&&) -> TimestampLiteral& = delete;

    // Destructor
    ~TimestampLiteral() override = default;

    // Methods inherited from Value
    auto print() const -> void override;

    // Methods inherited from Literal
    [[nodiscard]] auto matches_type(LiteralType type) const -> bool override {
        return 0 != (type & cTimestampLiteralTypes);
    }

    [[nodiscard]] auto matches_any(literal_type_bitmask_t mask) const -> bool override {
        return 0 != (mask & cTimestampLiteralTypes);
    }

    [[nodiscard]] auto matches_exactly(literal_type_bitmask_t mask) const -> bool override {
        return mask == cTimestampLiteralTypes;
    }

    /**
     * Checks compatibility with numeric and timestamp value types.
     * @param type The single value type to test.
     * @param op The comparison operation; unused.
     * @return Whether `type` is integer, float, or timestamp.
     */
    [[nodiscard]] auto can_match_op(LiteralType type, FilterOperation /*op*/) const
            -> bool override {
        return LiteralType::IntegerT == type || LiteralType::FloatT == type
               || LiteralType::TimestampT == type;
    }

    auto as_timestamp() -> bool override { return true; }

    auto as_int(int64_t& ret, FilterOperation op) -> bool override;

    auto as_float(double& ret, FilterOperation op) -> bool override;

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
    auto set_default_precision(Precision precision) -> void;

private:
    // Types
    static constexpr literal_type_bitmask_t cTimestampLiteralTypes{TimestampT};

    // Constructors
    explicit TimestampLiteral(epochtime_t timestamp);

    // Data members
    epochtime_t m_timestamp{};
    epochtime_t m_default_precision_timestamp{};
    double m_double_timestamp{};
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_TIMESTAMPLITERAL_HPP
