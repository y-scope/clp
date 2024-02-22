#ifndef CLP_AGGREGATION_COUNT_OPERATOR_HPP
#define CLP_AGGREGATION_COUNT_OPERATOR_HPP

#include <map>
#include <string>

#include "GroupTags.hpp"
#include "Operator.hpp"

namespace reducer {
/**
 * Basic count operator which will accumulate counts within different tagged groups.
 */
class CountOperator : public Operator {
public:
    [[nodiscard]] OperatorType get_type() const override { return OperatorType::Reduce; }

    [[nodiscard]] OperatorResultCardinality get_cardinality() const override {
        return OperatorResultCardinality::One;
    }

    void push_intra_stage_record_group(RecordGroup const& record_group) override;

    void push_inter_stage_record_group(RecordGroup const& record_group) override;

    std::unique_ptr<RecordGroupIterator> get_stored_result_iterator() override;
    std::unique_ptr<RecordGroupIterator> get_stored_result_iterator(
            std::set<GroupTags> const& filtered_tags
    ) override;

private:
    std::map<GroupTags, int64_t> m_group_count;
};
}  // namespace reducer

#endif  // CLP_AGGREGATION_COUNT_OPERATOR_HPP
