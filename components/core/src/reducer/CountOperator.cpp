#include "CountOperator.hpp"

namespace reducer {
void CountOperator::push_intra_stage_record_group(
        GroupTags const& tags,
        ConstRecordIterator& record_it
) {
    auto& count = m_group_count[tags];

    for (; !record_it.done(); record_it.next()) {
        count += record_it.get().get_int64_value("count");
    }
}

void CountOperator::push_inter_stage_record_group(
        GroupTags const& tags,
        ConstRecordIterator& record_it
) {
    auto& count = m_group_count[tags];

    for (; !record_it.done(); record_it.next()) {
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
