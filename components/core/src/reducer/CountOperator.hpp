#ifndef REDUCER_COUNTOPERATOR_HPP
#define REDUCER_COUNTOPERATOR_HPP

#include <map>
#include <string>

#include "GroupTags.hpp"
#include "Operator.hpp"

namespace reducer {
/**
 * Count operator that accumulates a count per record group.
 */
class CountOperator : public Operator {
public:
    static constexpr char cRecordElementKey[] = "count";

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

#endif  // REDUCER_COUNTOPERATOR_HPP
