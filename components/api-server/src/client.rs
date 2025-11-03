use clp_rust_utils::{
    clp_config::package::{
        config::{Config, StorageEngine},
        credentials::Credentials,
    },
    database::mysql::create_mysql_pool,
    job_config::{QUERY_JOBS_TABLE_NAME, QueryJobStatus, QueryJobType, SearchJobConfig},
};
use futures::{Stream, StreamExt};
use num_enum::TryFromPrimitive;
use serde::{Deserialize, Serialize};
use sqlx::Row;
use thiserror::Error;

#[derive(Clone)]
pub struct Client {
    #[allow(clippy::struct_field_names)]
    mongodb_client: mongodb::Client,
    sql_pool: sqlx::Pool<sqlx::MySql>,
    config: Config,
}

impl Client {
    /// Establishes a connection to both the `MySQL` database and the `MongoDB` database,
    /// creating a new `Self` (Client) instance.
    ///
    /// # Arguments
    ///
    /// * `config` - A `Config` struct containing database and cache configuration details.
    /// * `credentials` - A `Credentials` struct containing sensitive database login information.
    ///
    /// # Errors
    ///
    /// Returns a `ClientError` if either the `MySQL` connection pool creation or
    /// the `MongoDB` client initialization fails. This includes networking issues,
    /// invalid credentials, or incorrect configuration settings.
    ///
    /// # Returns
    ///
    /// A `Result` which is `Ok(Self)` (the connected `Client` instance) on success,
    /// or `Err(ClientError)` on failure.
    pub async fn connect(config: &Config, credentials: &Credentials) -> Result<Self, ClientError> {
        let sql_pool = create_mysql_pool(&config.database, &credentials.database, 10).await?;

        let mongo_uri = format!(
            "mongodb://{}:{}/{}?directConnection=true",
            config.results_cache.host, config.results_cache.port, config.results_cache.db_name,
        );
        let mongo_client = mongodb::Client::with_uri_str(mongo_uri).await?;

        Ok(Self {
            config: config.clone(),
            mongodb_client: mongo_client,
            sql_pool,
        })
    }

    /// Submits a search or aggregation query to be processed as a job.
    ///
    /// # Arguments
    ///
    /// * `query_config` - The configuration for the search or aggregation query.
    ///
    /// # Returns
    ///
    /// A `Result` which is:
    ///
    /// * `Ok(u64)` - The unique ID of the newly created query job on success.
    /// * `Err(ClientError)` - An error if the serialization fails or the database operation fails.
    ///
    /// # Errors
    ///
    /// This function can return a `ClientError` if:
    /// * The `query_config` fails to be serialized into a `MessagePack` byte vector.
    /// * The SQL execution against the database fails.
    pub async fn submit_query(&self, query_config: QueryConfig) -> Result<u64, ClientError> {
        let mut search_job_config: SearchJobConfig = query_config.into();
        if search_job_config.dataset.is_none() {
            search_job_config.dataset = match self.config.package.storage_engine {
                StorageEngine::Clp => None,
                StorageEngine::ClpS => Some("default".to_owned()),
            }
        }
        if search_job_config.max_num_results == 0 {
            search_job_config.max_num_results = 1000;
        }

        let query_job_type_i32: i32 = QueryJobType::SearchOrAggregation.into();
        let query_result = sqlx::query(&format!(
            "INSERT INTO `{QUERY_JOBS_TABLE_NAME}` (`job_config`, `type`) VALUES (?, ?)"
        ))
        .bind(rmp_serde::to_vec_named(&search_job_config)?)
        .bind(query_job_type_i32)
        .execute(&self.sql_pool)
        .await?;

        let search_job_id = query_result.last_insert_id();
        Ok(search_job_id)
    }

    /// Asynchronously fetches the results of a completed search job.
    ///
    /// # Arguments
    ///
    /// * `search_job_id` - The unique identifier of the search job whose results are to be fetched.
    ///
    /// # Returns
    ///
    /// A `Result` that contains:
    /// * `Ok(impl Stream<Item = Result<serde_json::Value, ClientError>>)`: A stream of the job's
    ///   results. Each item in the stream is a `Result` indicating either the successfully parsed
    ///   `serde_json::Value` or a `ClientError`.
    /// * `Err(ClientError)`: An error if the initial status check or the database operation fails,
    ///   or if the job status is ultimately `Failed`, `Cancelled`, or `Killed`.
    ///
    /// # Errors
    ///
    /// This function can return the following errors wrapped in `ClientError`:
    /// * `ClientError::QueryNotSucceeded`: If the search job status is `Failed`, `Cancelled`, or
    ///   `Killed`.
    /// * Errors related to `self.get_status()` or `MongoDB` operations (e.g., connection, find).
    /// * `ClientError::DecodeError`: If a retrieved document does not contain a "message" field, if
    ///   the "message" field is not a BSON string, or if the JSON string fails to parse.
    pub async fn fetch_results(
        &self,
        search_job_id: u64,
    ) -> Result<impl Stream<Item = Result<serde_json::Value, ClientError>> + use<>, ClientError>
    {
        loop {
            match self.get_status(search_job_id).await? {
                QueryJobStatus::Succeeded => {
                    break;
                }
                QueryJobStatus::Failed | QueryJobStatus::Cancelled | QueryJobStatus::Killed => {
                    return Err(ClientError::QueryNotSucceeded);
                }
                QueryJobStatus::Running | QueryJobStatus::Pending | QueryJobStatus::Cancelling => {
                    tokio::time::sleep(tokio::time::Duration::from_millis(500)).await;
                }
            }
        }

        let database = self
            .mongodb_client
            .database(&self.config.results_cache.db_name);
        let collection: mongodb::Collection<mongodb::bson::Document> =
            database.collection(&search_job_id.to_string());
        let cursor = collection.find(mongodb::bson::doc! {}).await?;

        let mapped = cursor.map(|res| {
            let doc = res?;
            let Some(msg) = doc.get("message") else {
                return Err(ClientError::MalformedData);
            };
            let mongodb::bson::Bson::String(message) = msg else {
                return Err(ClientError::MalformedData);
            };
            let value: serde_json::Value = serde_json::from_str(message)?;
            Ok(value)
        });

        Ok(mapped)
    }

    /// Retrieves the current status of a previously submitted search job.
    ///
    /// # Arguments
    ///
    /// * `search_job_id` - The unique identifier (`u64`) of the search job whose status is being
    ///   requested.
    ///
    /// # Returns
    ///
    /// A `Result` which is either:
    ///
    /// * `Ok(QueryJobStatus)`: The current status of the job, wrapped in the `QueryJobStatus` enum.
    /// * `Err(ClientError)`: An error if the database query fails, the job ID is not found, or if
    ///   the status value cannot be converted into a valid `QueryJobStatus`.
    ///
    /// # Errors
    ///
    /// This function can return a `ClientError` for reasons including:
    /// * **Database Errors:** Failure to execute the SQL query (e.g., connection issues).
    /// * **Row Not Found:** If no job with the given `search_job_id` exists.
    /// * **Data Conversion Errors:** If the retrieved status integer is invalid or cannot be
    ///   converted to `QueryJobStatus`.
    pub async fn get_status(&self, search_job_id: u64) -> Result<QueryJobStatus, ClientError> {
        let row = sqlx::query(&format!(
            "SELECT status FROM `{QUERY_JOBS_TABLE_NAME}` WHERE id = ?"
        ))
        .bind(search_job_id)
        .fetch_one(&self.sql_pool)
        .await?;
        let status: i32 = row.try_get("status")?;
        Ok(QueryJobStatus::try_from(status)?)
    }
}

#[derive(Clone, Serialize, Deserialize)]
pub struct QueryConfig {
    pub query_string: String,
    #[serde(default)]
    pub dataset: Option<String>,
    #[serde(default)]
    pub max_num_results: u32,
    #[serde(default)]
    pub begin_timestamp: Option<i64>,
    #[serde(default)]
    pub end_timestamp: Option<i64>,
    #[serde(default)]
    pub ignore_case: bool,
}

impl From<QueryConfig> for SearchJobConfig {
    fn from(value: QueryConfig) -> Self {
        Self {
            dataset: value.dataset,
            query_string: value.query_string,
            max_num_results: value.max_num_results,
            begin_timestamp: value.begin_timestamp,
            end_timestamp: value.end_timestamp,
            ignore_case: value.ignore_case,
            ..Default::default()
        }
    }
}

#[derive(Error, Debug)]
pub enum ClientError {
    #[error("SQL error: {0}")]
    Sql(#[from] sqlx::Error),

    #[error("MongoDB error: {0}")]
    Mongo(#[from] mongodb::error::Error),

    #[error("Query is not succeeded")]
    QueryNotSucceeded,

    #[error("IO error")]
    Io(#[from] std::io::Error),

    #[error("Malformed database")]
    MalformedData,
}

trait IsMalformedData {}
impl IsMalformedData for serde_json::Error {}
impl IsMalformedData for rmp_serde::encode::Error {}
impl<Enum: TryFromPrimitive> IsMalformedData for num_enum::TryFromPrimitiveError<Enum> {}

impl<T: IsMalformedData> From<T> for ClientError {
    fn from(_value: T) -> Self {
        Self::MalformedData
    }
}

impl From<clp_rust_utils::Error> for ClientError {
    fn from(value: clp_rust_utils::Error) -> Self {
        match value {
            clp_rust_utils::Error::MsgpackEncode(_) => Self::MalformedData,
            clp_rust_utils::Error::Io(error) => error.into(),
            clp_rust_utils::Error::Sqlx(error) => error.into(),
        }
    }
}
