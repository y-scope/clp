#include "RecordGroupSerdes.hpp"

#include <string>

#include <json/single_include/nlohmann/json.hpp>

namespace reducer {
DeserializedRecordGroup::DeserializedRecordGroup(std::vector<uint8_t>& serialized_data)
        : m_record_group{nlohmann::json::from_msgpack(serialized_data)},
          m_record_it{m_record_group["records"].template get<nlohmann::json::array_t>()} {
    init_tags_from_json();
}

DeserializedRecordGroup::DeserializedRecordGroup(char* buf, size_t len)
        : m_record_group{nlohmann::json::from_msgpack(buf, buf + len)},
          m_record_it{m_record_group["records"].template get<nlohmann::json::array_t>()} {
    init_tags_from_json();
}

void DeserializedRecordGroup::init_tags_from_json() {
    auto jtags = m_record_group["group_tags"].template get<nlohmann::json::array_t>();
    for (auto& jtag : jtags) {
        m_tags.emplace_back(jtag.template get<std::string>());
    }
}

std::vector<uint8_t> serialize(
        GroupTags const& tags,
        ConstRecordIterator& record_it,
        std::vector<uint8_t>(ser)(nlohmann::json const& j)
) {
    nlohmann::json json;
    json["group_tags"] = tags;
    auto records = nlohmann::json::array();

    for (; !record_it.done(); record_it.next()) {
        nlohmann::json record;
        for (auto typed_key_it = record_it.get().typed_key_iter(); !typed_key_it->done();
             typed_key_it->next())
        {
            TypedRecordKey typed_key = typed_key_it->get();
            auto key = typed_key.get_key();
            switch (typed_key.get_type()) {
                case ValueType::Int64:
                    record[key] = record_it.get().get_int64_value(key);
                    break;
                case ValueType::String:
                    record[key] = record_it.get().get_string_view(key);
                    break;
                case ValueType::Double:
                    record[key] = record_it.get().get_double_value(key);
                    break;
            }
        }
        records.push_back(std::move(record));
    }
    json["records"] = std::move(records);

    return std::move(ser(json));
}

std::vector<uint8_t> serialize_timeline(GroupTags const& tags, ConstRecordIterator& record_it) {
    nlohmann::json json;
    json["timestamp"] = std::stoll(tags[0]);
    auto records = nlohmann::json::array();

    int64_t count = 0;
    for (; !record_it.done(); record_it.next()) {
        count = record_it.get().get_int64_value("count");
    }
    json["count"] = count;

    return nlohmann::json::to_bson(json);
}

DeserializedRecordGroup deserialize(std::vector<uint8_t>& data) {
    return DeserializedRecordGroup{data};
}

DeserializedRecordGroup deserialize(char* buf, size_t len) {
    return {buf, len};
}
}  // namespace reducer
