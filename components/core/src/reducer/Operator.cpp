#include "Operator.hpp"

namespace reducer {
void Operator::finish() {
    if (nullptr == m_next_stage) {
        return;
    }

    for (auto it = get_stored_result_iterator(); false == it->done(); it->next()) {
        auto& group = it->get();
        m_next_stage->push_inter_stage_record_group(group.get_tags(), group.record_iter());
    }
}
}  // namespace reducer
