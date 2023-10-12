#include "Operator.hpp"

void Operator::finish() {
    if (next_stage_ == nullptr)
        return;
    
    for (auto it = this->get_stored_result_iterator(); !it->done(); it->next()) {
        next_stage_->push_inter_stage_record_group(*it->get());
    }
}