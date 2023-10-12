#include "CountOperator.hpp"

void CountOperator::push_intra_stage_record_group(RecordGroup const& record_group) {
    auto& count = group_count_[record_group.get_tags()];

    for (auto it = record_group.record_it(); !it->done(); it->next()) {
        count += it->get()->get_int64_value("count");
    }
}

void CountOperator::push_inter_stage_record_group(RecordGroup const& record_group) {
    auto& count = group_count_[record_group.get_tags()];

    for (auto it = record_group.record_it(); !it->done(); it->next()) {
        count += 1;
    }
}

std::unique_ptr<RecordGroupIterator> CountOperator::get_stored_result_iterator() {
    return std::unique_ptr<RecordGroupIterator>(new Int64MapRecordGroupIterator(group_count_, "count"));
}
