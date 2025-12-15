use anyhow::Result;
use async_trait::async_trait;
use rdkafka::{Message, consumer::StreamConsumer};

#[async_trait]
pub trait KafkaMessageReceiver: Send + Sync + 'static {
    /// Receives the next message from the Kafka stream.
    ///
    /// # Returns
    ///
    /// The message payload as a string on success.
    ///
    /// # Errors
    ///
    /// Returns an error if receiving the next message fails.
    async fn recv(&self) -> Result<String>;
}

pub struct KafkaConsumerWrapper {
    consumer: StreamConsumer,
}

impl From<StreamConsumer> for KafkaConsumerWrapper {
    fn from(consumer: StreamConsumer) -> Self {
        Self { consumer }
    }
}

impl KafkaConsumerWrapper {
    /// Creates a new Kafka consumer wrapper.
    ///
    /// # Errors
    ///
    /// Forwards errors from [`clp_rust_utils::kafka::create_new_consumer`].
    pub fn create(brokers: Vec<String>, group_id: String, topics: Vec<String>) -> Result<Self> {
        let consumer = clp_rust_utils::kafka::create_new_consumer(brokers, group_id, topics)?;
        Ok(Self::from(consumer))
    }
}

#[async_trait]
impl KafkaMessageReceiver for KafkaConsumerWrapper {
    async fn recv(&self) -> Result<String> {
        let borrowed_message = self.consumer.recv().await?;
        let message = borrowed_message.detach();
        match message.payload() {
            Some(payload) => Ok(String::from_utf8_lossy(payload).into_owned()),
            None => Err(anyhow::anyhow!("Received a Kafka message with no payload")),
        }
    }
}
