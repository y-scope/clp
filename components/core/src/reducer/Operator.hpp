#ifndef REDUCER_OPERATOR_HPP
#define REDUCER_OPERATOR_HPP

#include <memory>
#include <set>
#include <utility>

#include "RecordGroup.hpp"
#include "RecordGroupIterator.hpp"

namespace reducer {
/**
 * Class implementing a generic Operator that operates on RecordGroup objects.
 *
 * Operators implement methods to accept inter-stage RecordGroups and intra-stage RecordGroups.
 * In the former case, the operator acts like a combiner, combining records from a previous pipeline
 * stage. In the latter case, the operator acts like a reducer, reducing pre-aggregated results from
 * within a pipeline stage.
 */
class Operator {
public:
    virtual ~Operator() = default;

    virtual void
    push_intra_stage_record_group(GroupTags const& tags, ConstRecordIterator& record_it)
            = 0;
    virtual void
    push_inter_stage_record_group(GroupTags const& tags, ConstRecordIterator& record_it)
            = 0;

    void set_next_stage(std::shared_ptr<Operator> next_operator) {
        m_next_stage = std::move(next_operator);
    }

    // TODO: Default implementation of finish
    void finish();

    virtual std::unique_ptr<RecordGroupIterator> get_stored_result_iterator() = 0;

    virtual std::unique_ptr<RecordGroupIterator> get_stored_result_iterator(
            [[maybe_unused]] std::set<GroupTags> const& filtered_tags
    ) {
        // TODO: By default operators don't have to support filtering their output. We should
        // implement an iterator that wraps RecordGroupIterators and performs the filtering when the
        // underlying operator doesn't.
        return get_stored_result_iterator();
    }

protected:
    std::shared_ptr<Operator> m_next_stage;
};
}  // namespace reducer

#endif  // REDUCER_OPERATOR_HPP
