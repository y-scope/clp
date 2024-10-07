#include <librdkafka/rdkafka.h>

#include <functional>
#include <string>

#include "TraceableException.hpp"

namespace clp_s {
/**
 * This class provides a high-level interface to consume messages from a Kafka topic starting at
 * a given partition and offset.
 */
class KafkaReader {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructors
    /**
     * KafkaReader constructor.
     * @param config_file YAML configuration file for the Kafka consumer.
     * @param topic topic to consume from.
     * @param partition partition to consume from.
     * @param offset starting offset to consume from.
     * @throws KafkaReader::OperationFailed
     */
    explicit KafkaReader(
            std::string const& config_file,
            std::string const& topic,
            int32_t partition,
            int64_t offset
    );

    // Destructor
    ~KafkaReader();

    // Disable copy/move constructors/assignment operators
    KafkaReader(KafkaReader&) = delete;
    KafkaReader(KafkaReader&&) = delete;
    auto operator=(KafkaReader&) -> KafkaReader& = delete;
    auto operator=(KafkaReader&&) -> KafkaReader& = delete;

    /**
     * Consume up to num_messages messages from this KafkaReader.
     * @param consume a function which accepts the raw bytes and payload length for each message
     * @param num_messages the number of messages to consume
     * @return the number of messages consumed, or -1 on error
     */
    auto
    consume_messages(std::function<void(char*, size_t)> consume, size_t num_messages) -> ssize_t;

private:
    /**
     * Populate Kafka consumer configuration from a YAML config.
     *
     * String key-value pairs should be specified under a top-level "kafka" object. e.g.
     *
     * kafka:
     *   boostrap.servers: "localhost:3001,localhost:3002"
     *
     * All options from the Global configuration properties from the following link should work:
     * https://docs.confluent.io/platform/current/clients/librdkafka/html/md_CONFIGURATION.html
     * @param config_file path to YAML config file
     * @param conf kafka configuration object to populate
     * @return true on success and false otherwise
     */
    static auto
    populate_config_from_yaml_file(std::string const& config_file, rd_kafka_conf_t* conf) -> bool;

    rd_kafka_t* m_consumer{nullptr};
    rd_kafka_topic_t* m_topic{nullptr};
    int32_t m_partition{};
};
}  // namespace clp_s
