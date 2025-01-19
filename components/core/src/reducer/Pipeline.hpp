#ifndef REDUCER_PIPELINE_HPP
#define REDUCER_PIPELINE_HPP

#include <memory>
#include <set>
#include <vector>

#include "GroupTags.hpp"
#include "Operator.hpp"
#include "Record.hpp"
#include "RecordGroup.hpp"

namespace reducer {
enum class PipelineInputMode : uint8_t {
    IntraStage,
    InterStage
};

/**
 * An in-memory aggregation pipeline consisting of set of Operator objects chained together.
 */
class Pipeline {
public:
    explicit Pipeline(PipelineInputMode input_mode) : m_input_mode{input_mode} {}

    void push_record(Record const& record);
    void push_record_group(GroupTags const& tags, ConstRecordIterator& record_it);

    void add_pipeline_stage(std::shared_ptr<Operator> const& op);

    std::unique_ptr<RecordGroupIterator> finish();
    std::unique_ptr<RecordGroupIterator> finish(std::set<GroupTags> const& filtered_tags);

private:
    std::vector<std::shared_ptr<Operator>> m_stages;
    PipelineInputMode m_input_mode;
    GroupTags m_empty_group_tags;
};
}  // namespace reducer

#endif  // REDUCER_PIPELINE_HPP
