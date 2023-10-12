#ifndef CLP_AGGREGATION_COUNT_OPERATOR_HPP
#define CLP_AGGREGATION_COUNT_OPERATOR_HPP

#include <map>
#include <string>

//
#include "GroupTags.hpp"
#include "Operator.hpp"

class CountOperator : public Operator {
public:
    CountOperator() : Operator() {}

    virtual OperatorType get_type() const { return OperatorType::REDUCE; }

    virtual OperatorResultCardinality get_cardinality() const {
        return OperatorResultCardinality::ONE;
    }

    virtual void push_intra_stage_record_group(RecordGroup const& record_group);

    virtual void push_inter_stage_record_group(RecordGroup const& record_group);

    virtual std::unique_ptr<RecordGroupIterator> get_stored_result_iterator();

private:
    std::map<GroupTags, int64_t> group_count_;
};

#endif  // CLP_AGGREGATION_COUNT_OPERATOR_HPP
