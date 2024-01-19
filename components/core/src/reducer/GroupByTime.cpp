#include "GroupByTime.hpp"

#include "RecordGroup.hpp"

namespace reducer {
void GroupByTime::push_inter_stage_record_group(RecordGroup const& record_group) {
    for (auto it = record_group.record_it(); !it->done(); it->next()) {
        int64_t time = it->get()->get_int64_value("@time");
        time = time / m_bucket_size;
        time = time * m_bucket_size;
        if (time != m_prev_time) {
            m_tags[0] = std::to_string(time);
            m_prev_time = time;
        }

        m_next_stage->push_inter_stage_record_group(BasicSingleRecordGroup(&m_tags, &m_empty));
    }
}
}  // namespace reducer
