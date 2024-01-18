#include "CountOperator.hpp"

namespace reducer {
void CountOperator::push_intra_stage_record_group(RecordGroup const& record_group) {
    auto& count = m_group_count[record_group.get_tags()];

    for (auto it = record_group.record_it(); !it->done(); it->next()) {
        count += it->get()->get_int64_value("count");
    }
}

void CountOperator::push_inter_stage_record_group(RecordGroup const& record_group) {
    auto& count = m_group_count[record_group.get_tags()];

    for (auto it = record_group.record_it(); !it->done(); it->next()) {
        count += 1;
    }
}

std::unique_ptr<RecordGroupIterator> CountOperator::get_stored_result_iterator() {
    return std::make_unique<Int64MapRecordGroupIterator>(m_group_count, "count");
}

std::unique_ptr<RecordGroupIterator> CountOperator::get_stored_result_iterator(
        std::set<GroupTags> const& filtered_tags
) {
    return std::make_unique<FilteredInt64MapRecordGroupIterator>(
            m_group_count,
            filtered_tags,
            "count"
    );
}
}  // namespace reducer
