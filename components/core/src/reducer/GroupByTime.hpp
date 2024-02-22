#ifndef CLP_AGGREGATION_GROUPBY_TIME_OPERATOR_HPP
#define CLP_AGGREGATION_GROUPBY_TIME_OPERATOR_HPP

#include <map>
#include <string>

#include "GroupTags.hpp"
#include "Operator.hpp"
#include "Record.hpp"

namespace reducer {
/**
 * Operator that takes in records and forwards them down the pipeline with a group tag determined
 * by their "@time" attribute.
 *
 * The group tag is created based on the configured "bucket size" in milliseconds which will align
 * timestamps to n * "bucket size" milliseconds since epoch boundaries (taking the nearest boundary
 * lower than the provided timestamp).
 *
 * TODO: the current implementation throws out all attributes of the record and its group tags which
 * is fine for now because we only need it for local in memory group by -> count, but this operator
 * should be generalized in the future.
 */
class GroupByTime : public Operator {
public:
    explicit GroupByTime(int64_t bucket_size = 5 * 60 * 1000) : m_bucket_size(bucket_size) {
        m_tags.emplace_back("0");
    }

    [[nodiscard]] OperatorType get_type() const override { return OperatorType::GroupBy; }

    [[nodiscard]] OperatorResultCardinality get_cardinality() const override {
        return OperatorResultCardinality::Input;
    }

    void push_intra_stage_record_group(RecordGroup const& record_group) override {
        push_inter_stage_record_group(record_group);
    }

    void push_inter_stage_record_group(RecordGroup const& record_group) override;

    std::unique_ptr<RecordGroupIterator> get_stored_result_iterator() override {
        return std::make_unique<EmptyRecordGroupIterator>();
    }

private:
    EmptyRecord m_empty;
    GroupTags m_tags;
    int64_t m_prev_time{-1};
    int64_t m_bucket_size;
};
}  // namespace reducer

#endif  // CLP_AGGREGATION_GROUPBY_TIME_OPERATOR_HPP
