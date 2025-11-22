use std::net::SocketAddr;

use serde_json::{Value, json};
use tracing::info;

use crate::models::TokenSubject;

const COMPONENT_NAME: &str = "credential-manager";

/// Outcome associated with an audit record.
#[derive(Debug, Clone, Copy)]
pub enum AuditStatus {
    Success,
    Failure,
}

impl AuditStatus {
    /// Returns the lowercase form consumed by downstream log processors.
    fn as_str(self) -> &'static str {
        match self {
            AuditStatus::Success => "success",
            AuditStatus::Failure => "failure",
        }
    }
}

/// Structured payload describing a single credential-related action.
#[derive(Debug)]
pub struct AuditEvent<'a> {
    pub event_type: &'a str,
    pub credential_id: Option<i64>,
    pub credential_name: Option<&'a str>,
    pub subject: Option<&'a TokenSubject>,
    pub source_ip: Option<&'a SocketAddr>,
    pub status: AuditStatus,
    pub error: Option<&'a str>,
    pub metadata: Option<Value>,
}

impl<'a> AuditEvent<'a> {
    /// Helper for constructing a success event with default metadata.
    pub fn success(event_type: &'a str) -> Self {
        Self {
            event_type,
            credential_id: None,
            credential_name: None,
            subject: None,
            source_ip: None,
            status: AuditStatus::Success,
            error: None,
            metadata: None,
        }
    }

    /// Helper for constructing a failure event that records an error string.
    pub fn failure(event_type: &'a str, error: &'a str) -> Self {
        Self {
            event_type,
            credential_id: None,
            credential_name: None,
            subject: None,
            source_ip: None,
            status: AuditStatus::Failure,
            error: Some(error),
            metadata: None,
        }
    }
}

/// Emits the provided audit event to the tracing ecosystem using the `audit` target.
pub fn log_event(event: AuditEvent<'_>) {
    let subject = event
        .subject
        .and_then(|subject| serde_json::to_value(subject).ok())
        .unwrap_or(Value::Null);
    let source_ip = event
        .source_ip
        .map(|addr| addr.ip().to_string())
        .unwrap_or_default();
    let payload = json!({
        "component": COMPONENT_NAME,
        "event_type": event.event_type,
        "status": event.status.as_str(),
        "credential_id": event.credential_id,
        "credential_name": event.credential_name,
        "subject": subject,
        "source_ip": if source_ip.is_empty() { Value::Null } else { Value::String(source_ip) },
        "error": event.error,
        "metadata": event.metadata.unwrap_or(Value::Null),
    });

    info!(target = "audit", %payload);
}
