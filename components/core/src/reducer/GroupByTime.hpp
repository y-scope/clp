#ifndef CLP_AGGREGATION_GROUPBY_TIME_OPERATOR_HPP
#define CLP_AGGREGATION_GROUPBY_TIME_OPERATOR_HPP

#include <map>
#include <string>

#include "GroupTags.hpp"
#include "Operator.hpp"
#include "Record.hpp"

namespace reducer {
class GroupByTime : public Operator {
public:
    GroupByTime(int64_t bucket_size = 5 * 60 * 1000)
            : Operator(),
              m_bucket_size(bucket_size),
              m_prev_time(-1) {
        m_tags.push_back("0");
    }

    virtual OperatorType get_type() const { return OperatorType::GROUPBY; }

    virtual OperatorResultCardinality get_cardinality() const {
        return OperatorResultCardinality::INPUT;
    }

    virtual void push_intra_stage_record_group(RecordGroup const& record_group) {
        push_inter_stage_record_group(record_group);
    }

    virtual void push_inter_stage_record_group(RecordGroup const& record_group);

    virtual std::unique_ptr<RecordGroupIterator> get_stored_result_iterator() {
        return std::make_unique<EmptyRecordGroupIterator>();
    }

private:
    EmptyRecord m_empty;
    GroupTags m_tags;
    int64_t m_prev_time;
    int64_t m_bucket_size;
};
}  // namespace reducer

#endif  // CLP_AGGREGATION_GROUPBY_TIME_OPERATOR_HPP
