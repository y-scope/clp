#include "Pipeline.hpp"

#include "RecordGroup.hpp"

namespace reducer {
void Pipeline::push_record_group(GroupTags const& tags, ConstRecordIterator& record_it) {
    if (m_pipeline.size() > 0) {
        if (m_input_mode == PipelineInputMode::InterStage) {
            m_pipeline[0]->push_inter_stage_record_group(tags, record_it);
        } else /*input_mode == PipelineInputMode::IntraStage*/ {
            m_pipeline[0]->push_intra_stage_record_group(tags, record_it);
        }
    }
    // else silently drop
}

void Pipeline::push_record(Record const& record) {
    auto record_it = SingleRecordIterator(record);
    push_record_group(m_empty_group_tags, record_it);
}

void Pipeline::add_pipeline_stage(std::shared_ptr<Operator> op) {
    m_pipeline.push_back(op);
    if (m_pipeline.size() > 1) {
        m_pipeline[m_pipeline.size() - 2]->set_next_stage(op);
    }
}

std::unique_ptr<RecordGroupIterator> Pipeline::finish() {
    for (auto op = m_pipeline.begin(); op != m_pipeline.end(); ++op) {
        (*op)->finish();
    }

    if (!m_pipeline.empty()) {
        return m_pipeline.back()->get_stored_result_iterator();
    }

    return std::make_unique<EmptyRecordGroupIterator>();
}

std::unique_ptr<RecordGroupIterator> Pipeline::finish(std::set<GroupTags> const& filtered_tags) {
    // FIXME: assumes no need to push results between stages, but we will change the
    // programming model to eliminate the possibility of flushing between stages at the end later.
    if (!m_pipeline.empty()) {
        return m_pipeline.back()->get_stored_result_iterator(filtered_tags);
    }

    return std::make_unique<EmptyRecordGroupIterator>();
}
}  // namespace reducer
