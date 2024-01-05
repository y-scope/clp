#include "Operator.hpp"

namespace reducer {
void Operator::finish() {
    if (m_next_stage == nullptr) {
        return;
    }

    for (auto it = this->get_stored_result_iterator(); !it->done(); it->next()) {
        m_next_stage->push_inter_stage_record_group(*it->get());
    }
}
}  // namespace reducer
