#include "RecordGroupSerdes.hpp"

#include <iostream>
#include <string>

#include <json/single_include/nlohmann/json.hpp>

namespace reducer {
std::vector<uint8_t>
serialize(RecordGroup const& group, std::vector<uint8_t>(ser)(nlohmann::json const& j)) {
    nlohmann::json json;
    json["group_tags"] = group.get_tags();
    auto records = nlohmann::json::array();

    for (auto it = group.record_it(); !it->done(); it->next()) {
        nlohmann::json record;
        for (auto vit = it->get()->value_iter(); !vit->done(); vit->next()) {
            TypedRecordKey tk = vit->get();
            auto key = tk.get_key();
            switch (tk.get_type()) {
                case ValueType::Int64:
                    record[key] = it->get()->get_int64_value(key);
                    break;
                case ValueType::String:
                    record[key] = it->get()->get_string_view(key);
                    break;
                case ValueType::Double:
                    record[key] = it->get()->get_double_value(key);
                    break;
            }
        }
        records.push_back(std::move(record));
    }
    json["records"] = std::move(records);

    return std::move(ser(json));
}

std::vector<uint8_t> serialize_timeline(RecordGroup const& group) {
    nlohmann::json json;
    json["timestamp"] = std::stoll(group.get_tags()[0]);
    auto records = nlohmann::json::array();

    int64_t count = 0;
    for (auto it = group.record_it(); !it->done(); it->next()) {
        count = it->get()->get_int64_value("count");
    }
    json["count"] = count;

    return nlohmann::json::to_bson(json);
}

DeserializedRecordGroup deserialize(std::vector<uint8_t>& data) {
    return DeserializedRecordGroup(data);
}

DeserializedRecordGroup deserialize(char* buf, size_t len) {
    return {buf, len};
}

void DeserializedRecordGroup::init_tags_from_json() {
    auto jtags = m_record_group["group_tags"].template get<nlohmann::json::array_t>();
    for (auto& jtag : jtags) {
        m_tags.push_back(jtag.template get<std::string>());
    }
}

DeserializedRecordGroup::DeserializedRecordGroup(std::vector<uint8_t>& serialized_data)
        : m_record_group(nlohmann::json::from_msgpack(serialized_data)) {
    init_tags_from_json();
}

DeserializedRecordGroup::DeserializedRecordGroup(char* buf, size_t len)
        : m_record_group(nlohmann::json::from_msgpack(buf, buf + len)) {
    init_tags_from_json();
}

std::unique_ptr<RecordIterator> DeserializedRecordGroup::record_it() const {
    return std::make_unique<DeserializedRecordIterator>(
            m_record_group["records"].template get<nlohmann::json::array_t>()
    );
}

GroupTags const& DeserializedRecordGroup::get_tags() const {
    return m_tags;
}
}  // namespace reducer
