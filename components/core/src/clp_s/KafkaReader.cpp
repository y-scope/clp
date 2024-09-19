#include "KafkaReader.hpp"

#include <librdkafka/rdkafka.h>

#include <array>

#include <spdlog/spdlog.h>

namespace clp_s {
KafkaReader::KafkaReader(std::string const& topic, int32_t partition, int64_t offset)
        : m_partition(partition) {
    constexpr size_t cErrorBufferSize = 512;
    std::array<char, cErrorBufferSize> error_msg{};
    auto conf = rd_kafka_conf_new();

    // set_config(k, v, conf) multiple times

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
}  // namespace clp_s
