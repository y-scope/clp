#ifndef CLP_AGGREGATION_OPERATOR_HPP
#define CLP_AGGREGATION_OPERATOR_HPP

#include <memory>
#include <set>

#include "RecordGroup.hpp"
#include "RecordGroupIterator.hpp"

namespace reducer {
enum class OperatorType {
    MAP,
    GROUPBY,
    REDUCE
};

enum class OperatorResultCardinality {
    ONE,
    SUBSET,
    INPUT
};

class Operator {
public:
    Operator() : m_next_stage(nullptr) {}

    virtual ~Operator() = default;

    virtual OperatorType get_type() const = 0;
    virtual OperatorResultCardinality get_cardinality() const = 0;

    virtual void push_intra_stage_record_group(RecordGroup const& record_group) = 0;
    virtual void push_inter_stage_record_group(RecordGroup const& record_group) = 0;

    void set_next_stage(std::shared_ptr<Operator> next_operator) { m_next_stage = next_operator; }

    // TODO: default implementation of finish
    void finish();

    virtual std::unique_ptr<RecordGroupIterator> get_stored_result_iterator() = 0;

    virtual std::unique_ptr<RecordGroupIterator> get_stored_result_iterator(
            std::set<GroupTags> const& filtered_tags
    ) {
        return get_stored_result_iterator();
    }

protected:
    std::shared_ptr<Operator> m_next_stage;
};
}  // namespace reducer

#endif  // CLP_AGGREGATION_OPERATOR_HPP
