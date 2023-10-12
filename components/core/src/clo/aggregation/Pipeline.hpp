#ifndef CLP_AGGREGATION_PIPELINE_HPP
#define CLP_AGGREGATION_PIPELINE_HPP

#include <memory>
#include <vector>

// 
#include "Operator.hpp"
#include "Record.hpp"
#include "RecordGroup.hpp"
#include "GroupTags.hpp"

enum class PipelineInputMode {
    INTRA_STAGE,
    INTER_STAGE
};

class Pipeline {
public:
    Pipeline(PipelineInputMode input_mode) : input_mode_(input_mode) {};
    
    void push_record(Record const& record);
    void push_record_group(RecordGroup const& record_group);

    void add_pipeline_stage(std::shared_ptr<Operator> op);

    std::unique_ptr<RecordGroupIterator> finish();
private:
    std::vector<std::shared_ptr<Operator>> pipeline_;
    PipelineInputMode input_mode_;
    GroupTags empty_group_tags_;
};

#endif  // CLP_AGGREGATION_PIPELINE_HPP
