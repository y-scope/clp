#include "DeserializedRecordGroup.hpp"

#include <string>

#include <json/single_include/nlohmann/json.hpp>

namespace reducer {
DeserializedRecordGroup::DeserializedRecordGroup(std::vector<uint8_t>& serialized_data)
        : m_record_group(nlohmann::json::from_msgpack(serialized_data)),
          m_record_it(m_record_group[static_cast<char const*>(cRecordsKey)]
                              .template get<nlohmann::json::array_t>()) {
    init_tags_from_json();
}

DeserializedRecordGroup::DeserializedRecordGroup(char* buf, size_t len)
        : m_record_group(nlohmann::json::from_msgpack(buf, buf + len)),
          m_record_it(m_record_group[static_cast<char const*>(cRecordsKey)]
                              .template get<nlohmann::json::array_t>()) {
    init_tags_from_json();
}

void DeserializedRecordGroup::init_tags_from_json() {
    auto tags = m_record_group[static_cast<char const*>(cGroupTagsKey)]
                        .template get<nlohmann::json::array_t>();
    for (auto& tag : tags) {
        m_tags.emplace_back(tag.template get<std::string>());
    }
}

std::vector<uint8_t> serialize(
        GroupTags const& tags,
        ConstRecordIterator& record_it,
        std::vector<uint8_t>(serializer)(nlohmann::json const& j)
) {
    nlohmann::json json;
    json[static_cast<char const*>(DeserializedRecordGroup::cGroupTagsKey)] = tags;
    auto records = nlohmann::json::array();

    for (; false == record_it.done(); record_it.next()) {
        nlohmann::json record;
        for (auto typed_key_it = record_it.get().typed_key_iter(); false == typed_key_it->done();
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
        records.emplace_back(std::move(record));
    }
    json[static_cast<char const*>(DeserializedRecordGroup::cRecordsKey)] = std::move(records);

    return std::move(serializer(json));
}
}  // namespace reducer
