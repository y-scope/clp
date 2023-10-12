#include "Pipeline.hpp"

#include "RecordGroup.hpp"

void Pipeline::push_record_group(RecordGroup const& record_group) {
    if (pipeline_.size() > 0) {
        if (input_mode_ == PipelineInputMode::INTER_STAGE) {
            pipeline_[0]->push_inter_stage_record_group(record_group);
        } else /*input_mode == PipelineInputMode::INTRA_STAGE*/ {
            pipeline_[0]->push_intra_stage_record_group(record_group);
        }
    }
    // else silently drop
}

void Pipeline::push_record(Record const& record) {
    push_record_group(BasicSingleRecordGroup(&empty_group_tags_, &record));
}

void Pipeline::add_pipeline_stage(std::shared_ptr<Operator> op) {
    pipeline_.push_back(op);
    if (pipeline_.size() > 1) {
        pipeline_[pipeline_.size() - 2]->set_next_stage(op);
    }
}

std::unique_ptr<RecordGroupIterator> Pipeline::finish() {
    for (auto op = pipeline_.begin(); op != pipeline_.end(); ++op) {
        (*op)->finish();
    }

    if (!pipeline_.empty())
        return pipeline_.back()->get_stored_result_iterator();

    return std::unique_ptr<RecordGroupIterator>(new EmptyRecordGroupIterator());
}