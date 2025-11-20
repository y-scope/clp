use serde::Serialize;

#[derive(Debug, Clone, Serialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum TokenSubject {
    Job { job_id: i64 },
    Service { service_name: String },
}

impl TokenSubject {
    pub fn job(job_id: i64) -> Self {
        Self::Job { job_id }
    }

    pub fn service<S: Into<String>>(service_name: S) -> Self {
        Self::Service {
            service_name: service_name.into(),
        }
    }
}
