use serde::Serialize;

/// Identifies the entity requesting a token (job vs. service-level access).
#[derive(Debug, Clone, Serialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum TokenSubject {
    Job { job_id: i64 },
    Service { service_name: String },
}

impl TokenSubject {
    /// Convenience constructor for job-scoped tokens.
    pub fn job(job_id: i64) -> Self {
        Self::Job { job_id }
    }

    /// Convenience constructor for service-level tokens.
    pub fn service<S: Into<String>>(service_name: S) -> Self {
        Self::Service {
            service_name: service_name.into(),
        }
    }
}
