#ifndef CLP_AGGREGATION_OPERATOR_HPP
#define CLP_AGGREGATION_OPERATOR_HPP

#include <memory>

//
#include "RecordGroup.hpp"
#include "RecordGroupIterator.hpp"

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
    Operator() : next_stage_(nullptr) {}

    virtual ~Operator() {}

    virtual OperatorType get_type() const = 0;
    virtual OperatorResultCardinality get_cardinality() const = 0;

    virtual void push_intra_stage_record_group(RecordGroup const& record_group) = 0;
    virtual void push_inter_stage_record_group(RecordGroup const& record_group) = 0;

    void set_next_stage(std::shared_ptr<Operator> next_operator) { next_stage_ = next_operator; }

    // TODO: default implementation of finish
    void finish();

    virtual std::unique_ptr<RecordGroupIterator> get_stored_result_iterator() = 0;

protected:
    std::shared_ptr<Operator> next_stage_;
};

#endif  // CLP_AGGREGATION_OPERATOR_HPP
