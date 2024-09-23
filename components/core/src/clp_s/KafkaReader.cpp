#include "KafkaReader.hpp"

#include <librdkafka/rdkafka.h>

#include <array>
#include <stdexcept>

#include <spdlog/spdlog.h>
#include <yaml-cpp/include/yaml-cpp/yaml.h>

namespace clp_s {
auto KafkaReader::populate_config_from_yaml_file(
        std::string const& config_file,
        rd_kafka_conf_t* conf
) -> bool {
    constexpr char cKafkaConfig[] = "kafka";
    constexpr size_t cErrorBufferSize = 512;
    std::array<char, cErrorBufferSize> error_msg{};
    try {
        YAML::Node config = YAML::LoadFile(config_file);
        if (false == config[cKafkaConfig].IsDefined()) {
            return false;
        }
        auto kafka_config = config[cKafkaConfig];
        for (auto it = kafka_config.begin(); it != kafka_config.end(); ++it) {
            YAML::Node config_property_node = it->second;
            std::string const& key = config_property_node.Tag();
            std::string value = config_property_node.as<std::string>();

            auto rc = rd_kafka_conf_set(
                    conf,
                    key.c_str(),
                    value.c_str(),
                    error_msg.data(),
                    error_msg.size()
            );
            if (RD_KAFKA_CONF_OK != rc) {
                SPDLOG_ERROR(
                        "Failed to set Kafka config {}={}, error: {}",
                        key,
                        value,
                        error_msg.data()
                );
                return false;
            }
        }
    } catch (std::exception& e) {
        SPDLOG_ERROR("Failed to read YAML config for Kafka consumer: {}", e.what());
        return false;
    }
    return true;
}

KafkaReader::KafkaReader(
        std::string const& config_file,
        std::string const& topic,
        int32_t partition,
        int64_t offset
)
        : m_partition(partition) {
    constexpr size_t cErrorBufferSize = 512;
    std::array<char, cErrorBufferSize> error_msg{};
    auto conf = rd_kafka_conf_new();

    if (false == populate_config_from_yaml_file(config_file, conf)) {
        rd_kafka_conf_destroy(conf);
        throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
    }

    m_consumer = rd_kafka_new(RD_KAFKA_CONSUMER, conf, error_msg.data(), error_msg.size());
    if (nullptr == m_consumer) {
        rd_kafka_conf_destroy(conf);
        rd_kafka_destroy(m_consumer);
        SPDLOG_ERROR("Encountered error while creating kafka consumer: {}", error_msg.data());
        throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
    }
    // On success m_consumer takes ownership of conf, so we can stop tracking it here.
    conf = nullptr;

    rd_kafka_topic_conf_t* topic_conf{nullptr};
    m_topic = rd_kafka_topic_new(m_consumer, topic.c_str(), topic_conf);
    if (nullptr == m_topic) {
        auto err = rd_kafka_last_error();
        SPDLOG_ERROR("Encountered error while creating kafka topic: {}", rd_kafka_err2str(err));
        rd_kafka_topic_destroy(m_topic);
        rd_kafka_destroy(m_consumer);
        throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
    }
    // On success m_topic takes ownership of topic_conf, so we can stop tracking it here.
    topic_conf = nullptr;

    // This function returns -1 on error, but reports the specific error using thread local errno.
    constexpr int cKafkaConsumeStartErrorCode = -1;
    if (cKafkaConsumeStartErrorCode == rd_kafka_consume_start(m_topic, partition, offset)) {
        auto err = rd_kafka_last_error();
        SPDLOG_ERROR("Encountered error while starting kafka consumer: {}", rd_kafka_err2str(err));
        rd_kafka_topic_destroy(m_topic);
        rd_kafka_destroy(m_consumer);
        throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
    }
}

KafkaReader::~KafkaReader() {
    rd_kafka_consume_stop(m_topic, m_partition);
    rd_kafka_topic_destroy(m_topic);
    rd_kafka_destroy(m_consumer);
}

auto KafkaReader::consume_messages(std::function<void(char*, size_t)> consume, size_t num_messages)
        -> ssize_t {
    constexpr size_t cBatchSize = 128;
    constexpr int cTimeoutMs = 1000;
    ssize_t num_messages_consumed{0};
    std::array<rd_kafka_message_t*, cBatchSize> messages;

    bool end_of_partition{false};
    size_t num_messages_remaining{num_messages};
    do {
        size_t batch_size = num_messages < cBatchSize ? num_messages : cBatchSize;
        auto rc = rd_kafka_consume_batch(
                m_topic,
                m_partition,
                cTimeoutMs,
                messages.data(),
                batch_size
        );
        if (-1 == rc) {
            // Note: if we want to support backing off to longer timeouts we need to check if the
            // error is ETIMEDOUT and conditionally retry the consume batch request.
            auto err = rd_kafka_last_error();
            SPDLOG_ERROR(
                    "Encountered error while trying to consume batch from kafka: {}",
                    rd_kafka_err2str(err)
            );
            return -1;
        }

        bool error{false};
        for (size_t i = 0; i < rc; ++i) {
            rd_kafka_message_t* cur_message = messages[i];
            if (cur_message->err) {
                // Handle end of partition error
                if (RD_KAFKA_RESP_ERR__PARTITION_EOF != cur_message->err) {
                    end_of_partition = true;
                } else {
                    SPDLOG_ERROR(
                            "Encountered error while consuming kafka messages: {}",
                            rd_kafka_err2str(cur_message->err)
                    );
                    error = true;
                }

                rd_kafka_message_destroy(cur_message);
                continue;
            }

            num_messages_consumed += 1;
            consume(static_cast<char*>(cur_message->payload), cur_message->len);
            rd_kafka_message_destroy(cur_message);
        }

        // Defer returning error until after the loop to make sure rd_kafka_message_destroy is
        // called for every message.
        if (error) {
            return -1;
        }

        // This shouldn't happen outside of implementation or library bugs, but it is worth checking
        // for safety.
        if (num_messages_consumed > num_messages) {
            SPDLOG_ERROR(
                    "Received {} messages from Kafka when expecting {}",
                    num_messages_consumed,
                    num_messages
            );
            return -1;
        }
        num_messages_remaining = num_messages - num_messages_consumed;
    } while (0 < num_messages_remaining && false == end_of_partition);
    return num_messages_consumed;
}
}  // namespace clp_s
