#include "Pipeline.hpp"

#include "RecordGroup.hpp"

namespace reducer {
void Pipeline::push_record(Record const& record) {
    auto record_it = SingleRecordIterator(record);
    push_record_group(m_empty_group_tags, record_it);
}

void Pipeline::push_record_group(GroupTags const& tags, ConstRecordIterator& record_it) {
    if (m_stages.empty()) {
        // Silently drop the record group
        return;
    }

    if (PipelineInputMode::InterStage == m_input_mode) {
        m_stages.front()->push_inter_stage_record_group(tags, record_it);
    } else {  // PipelineInputMode::IntraStage == m_input_mode
        m_stages.front()->push_intra_stage_record_group(tags, record_it);
    }
}

void Pipeline::add_pipeline_stage(std::shared_ptr<Operator> const& op) {
    m_stages.emplace_back(op);
    if (m_stages.size() > 1) {
        m_stages[m_stages.size() - 2]->set_next_stage(op);
    }
}

std::unique_ptr<RecordGroupIterator> Pipeline::finish() {
    if (m_stages.empty()) {
        return std::make_unique<EmptyRecordGroupIterator>();
    }

    for (auto& m_stage : m_stages) {
        m_stage->finish();
    }

    return m_stages.back()->get_stored_result_iterator();
}

std::unique_ptr<RecordGroupIterator> Pipeline::finish(std::set<GroupTags> const& filtered_tags) {
    if (m_stages.empty()) {
        return std::make_unique<EmptyRecordGroupIterator>();
    }

    // TODO: This assumes there's no need to push results between stages; we'll change the
    // programming model to eliminate the possibility of flushing between stages later.
    return m_stages.back()->get_stored_result_iterator(filtered_tags);
}
}  // namespace reducer
