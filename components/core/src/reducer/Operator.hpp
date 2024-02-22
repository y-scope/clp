#ifndef CLP_AGGREGATION_OPERATOR_HPP
#define CLP_AGGREGATION_OPERATOR_HPP

#include <memory>
#include <set>
#include <utility>

#include "RecordGroup.hpp"
#include "RecordGroupIterator.hpp"

namespace reducer {
enum class OperatorType : uint8_t {
    Map,
    GroupBy,
    Reduce
};

enum class OperatorResultCardinality : uint8_t {
    One,
    Subset,
    Input
};

/**
 * Class implementing a generic Operator which operates on RecordGroup objects composed of GroupTags
 * and a list of Record objects.
 *
 * Operators implement methods to accept inter stage RecordGroups (operate on records from previous
 * pipeline stage i.e. a combiner) and intra stage RecordGroups (operate on pre-aggregated results
 * withing a pipeline stage i.e. a reducer).
 *
 * Operators can be of type GroupBy, Map, or Reduce, though currently no Map type Operators are
 * implemented.
 */
class Operator {
public:
    Operator() : m_next_stage(nullptr) {}

    virtual ~Operator() = default;

    [[nodiscard]] virtual OperatorType get_type() const = 0;
    [[nodiscard]] virtual OperatorResultCardinality get_cardinality() const = 0;

    virtual void push_intra_stage_record_group(RecordGroup const& record_group) = 0;
    virtual void push_inter_stage_record_group(RecordGroup const& record_group) = 0;

    void set_next_stage(std::shared_ptr<Operator> next_operator) {
        m_next_stage = std::move(next_operator);
    }

    // TODO: default implementation of finish
    void finish();

    virtual std::unique_ptr<RecordGroupIterator> get_stored_result_iterator() = 0;

    virtual std::unique_ptr<RecordGroupIterator> get_stored_result_iterator(
            [[maybe_unused]] std::set<GroupTags> const& filtered_tags
    ) {
        /** TODO: By default operators don't have to support filtering their output. We should
         *  implement an iterator that wraps RecordGroupIterators and performs the filtering when
         *  the underlying operator doesn't.
         */
        return get_stored_result_iterator();
    }

protected:
    std::shared_ptr<Operator> m_next_stage;
};
}  // namespace reducer

#endif  // CLP_AGGREGATION_OPERATOR_HPP
