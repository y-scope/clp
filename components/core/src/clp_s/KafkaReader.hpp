#include <librdkafka/rdkafka.h>

#include <functional>
#include <string>

#include "TraceableException.hpp"

namespace clp_s {

class KafkaReader {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructors
    explicit KafkaReader(std::string const& topic, int32_t partition, int64_t offset);

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
    auto consume_messages(std::function<char*, size_t> consume, size_t num_messages) -> ssize_t;

private:
    rd_kafka_t* m_consumer{nullptr};
    rd_kafka_topic_t* m_topic{nullptr};
    int32_t m_partition{};
};
}  // namespace clp_s
