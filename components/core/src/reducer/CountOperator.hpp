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
    void
    push_intra_stage_record_group(GroupTags const& tags, ConstRecordIterator& record_it) override;

    void
    push_inter_stage_record_group(GroupTags const& tags, ConstRecordIterator& record_it) override;

    std::unique_ptr<RecordGroupIterator> get_stored_result_iterator() override;
    std::unique_ptr<RecordGroupIterator> get_stored_result_iterator(
            std::set<GroupTags> const& filtered_tags
    ) override;

private:
    std::map<GroupTags, int64_t> m_group_count;
};
}  // namespace reducer

#endif  // CLP_AGGREGATION_COUNT_OPERATOR_HPP
