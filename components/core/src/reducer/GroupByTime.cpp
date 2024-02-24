#include "GroupByTime.hpp"

#include "RecordGroup.hpp"

namespace reducer {
void GroupByTime::push_inter_stage_record_group(
        GroupTags const& tags,
        ConstRecordIterator& record_it
) {
    for (; !record_it.done(); record_it.next()) {
        int64_t time = record_it.get().get_int64_value("@time");
        time = time / m_bucket_size;
        time = time * m_bucket_size;
        if (time != m_prev_time) {
            m_tags[0] = std::to_string(time);
            m_prev_time = time;
        }

        auto it = SingleRecordIterator(record_it.get());
        m_next_stage->push_inter_stage_record_group(m_tags, it);
    }
}
}  // namespace reducer
