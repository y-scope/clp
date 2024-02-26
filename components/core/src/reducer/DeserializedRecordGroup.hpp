#ifndef REDUCER_DESERIALIZEDRECORDGROUP_HPP
#define REDUCER_DESERIALIZEDRECORDGROUP_HPP

#include <iostream>
#include <utility>
#include <vector>

#include <json/single_include/nlohmann/json.hpp>

#include "ConstRecordIterator.hpp"
#include "JsonArrayRecordIterator.hpp"
#include "JsonRecord.hpp"
#include "Record.hpp"
#include "RecordGroup.hpp"
#include "RecordTypedKeyIterator.hpp"

namespace reducer {
/**
 * Class which converts serialized data into a RecordGroup and exposes iterators to the underlying
 * data.
 *
 * The serialized data comes from the "serialize" function declared in this file.
 */
class DeserializedRecordGroup : public RecordGroup {
public:
    static constexpr char cGroupTagsKey[] = "group_tags";
    static constexpr char cRecordsKey[] = "records";

    explicit DeserializedRecordGroup(std::vector<uint8_t>& serialized_data);
    DeserializedRecordGroup(char* buf, size_t len);

    [[nodiscard]] ConstRecordIterator& record_iter() override { return m_record_it; }

    [[nodiscard]] GroupTags const& get_tags() const override { return m_tags; }

private:
    void init_tags_from_json();

    GroupTags m_tags;
    nlohmann::json m_record_group;
    JsonArrayRecordIterator m_record_it;
};

/**
 * Converts a record group into a JSON object and then serializes it into a format that can be
 * deserialized by DeserializedRecordGroup.
 * @param tags The tags in the record group.
 * @param record_it An iterator for the records in the record group.
 * @param serializer The method to use for serializing the JSON object.
 * @return The serialized data.
 */
std::vector<uint8_t> serialize(
        GroupTags const& tags,
        ConstRecordIterator& record_it,
        std::vector<uint8_t>(serializer)(nlohmann::json const& j) = nlohmann::json::to_msgpack
);
}  // namespace reducer

#endif  // REDUCER_DESERIALIZEDRECORDGROUP_HPP
