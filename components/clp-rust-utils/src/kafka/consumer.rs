use rdkafka::{
    ClientConfig,
    consumer::{Consumer, StreamConsumer},
    error::KafkaError,
};

/// Creates a new Kafka consumer.
///
/// # Returns
///
/// A newly created Kafka StreamConsumer on success.
///
/// # Errors
///
/// Returns an error if:
/// * Kafka consumer creation fails.
/// * Kafka consumer fails to subscribe to the topics.
pub fn create_new_consumer(
    brokers: Vec<String>,
    group_id: String,
    topics: Vec<String>,
) -> Result<StreamConsumer, KafkaError> {
    let consumer: StreamConsumer = ClientConfig::new()
        .set("group.id", group_id)
        .set("bootstrap.servers", brokers.join(","))
        .set("auto.offset.reset", "latest")
        .set("enable.auto.commit", "true")
        .create()?;

    let topics_str = topics
        .iter()
        .map(|topic| topic.as_str())
        .collect::<Vec<&str>>();
    consumer.subscribe(&topics_str)?;
    Ok(consumer)
}
