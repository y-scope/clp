//! [`S3CompressionJobSubmitter`] implementation for [`spider_client::SpiderClient`].

use std::time::Duration;

use async_trait::async_trait;
use clp_rust_utils::{
    job_config::CompressionJobId,
    task_io::compression::{ClpSCompressionOption, S3InputSource},
};
use spider_client::{SpiderClient, error::ClientError};
use spider_core::{
    job::JobState,
    task::{
        DataTypeDescriptor,
        ExecutionPolicy,
        TaskDescriptor,
        TaskGraph,
        TdlContext,
        TerminationTaskDescriptor,
        ValueTypeDescriptor,
    },
    types::{
        id::{JobId, ResourceGroupId},
        io::TaskInput,
    },
};

use crate::{
    compression_job_submitter::{CompressionJobOutcome, S3CompressionJobSubmitter},
    error::Error,
};

#[async_trait]
impl S3CompressionJobSubmitter for SpiderClient {
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`TaskGraph::new`]'s return values on failure.
    /// * Forwards [`ValueTypeDescriptor::struct_from_name`]'s return values on failure.
    /// * Forwards [`TaskGraph::insert_task`]'s return values on failure.
    /// * Forwards [`rmp_serde::to_vec`]'s return values on failure.
    /// * Forwards [`SpiderClient::submit_job`]'s return values on failure.
    async fn submit_s3_compression_job(
        &self,
        compression_job_id: CompressionJobId,
        resource_group_id: ResourceGroupId,
        clp_s_option: ClpSCompressionOption,
        dataset: Option<String>,
        input_sources: Vec<(S3InputSource, ExecutionPolicy)>,
        commit_task_execution_policy: ExecutionPolicy,
    ) -> Result<JobId, Error> {
        // NOTE: These constants must be kept in sync with the TDL package definitions.
        const CLP_TDL_PACKAGE_NAME: &str = "clp";
        const COMPRESSION_TASK_FUNC: &str = "compression::clp_s_s3_compress";
        const COMMIT_TASK_FUNC: &str = "compression::commit";
        const COMPRESSION_TASK_NUM_INPUTS: usize = 3;

        let commit_task = TerminationTaskDescriptor {
            tdl_context: TdlContext {
                package: CLP_TDL_PACKAGE_NAME.to_owned(),
                task_func: COMMIT_TASK_FUNC.to_owned(),
            },
            execution_policy: Some(commit_task_execution_policy),
        };
        let mut graph = TaskGraph::new(Some(commit_task), None)?;

        let mut inputs: Vec<TaskInput> =
            Vec::with_capacity(input_sources.len() * COMPRESSION_TASK_NUM_INPUTS);
        for (input_source, execution_policy) in input_sources {
            graph.insert_task(TaskDescriptor {
                tdl_context: TdlContext {
                    package: CLP_TDL_PACKAGE_NAME.to_owned(),
                    task_func: COMPRESSION_TASK_FUNC.to_owned(),
                },
                execution_policy: Some(execution_policy),
                inputs: vec![
                    DataTypeDescriptor::Value(ValueTypeDescriptor::struct_from_name(
                        "ClpSCompressionOption",
                    )?),
                    DataTypeDescriptor::Value(ValueTypeDescriptor::struct_from_name(
                        "Option<String>",
                    )?),
                    DataTypeDescriptor::Value(ValueTypeDescriptor::struct_from_name(
                        "S3InputSource",
                    )?),
                ],
                outputs: vec![DataTypeDescriptor::Value(
                    ValueTypeDescriptor::struct_from_name("CompressionTaskOutput")?,
                )],
                input_sources: None,
            })?;
            inputs.push(TaskInput::ValuePayload(rmp_serde::to_vec(&clp_s_option)?));
            inputs.push(TaskInput::ValuePayload(rmp_serde::to_vec(&dataset)?));
            inputs.push(TaskInput::ValuePayload(rmp_serde::to_vec(&input_source)?));
        }

        let job_id = self.submit_job(resource_group_id, &graph, inputs).await?;

        tracing::info!(
            compression_job_id = % compression_job_id,
            spider_job_id = % job_id,
            num_tasks = graph.get_num_tasks(),
            "Submitted compression job to Spider."
        );

        Ok(job_id)
    }

    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`SpiderClient::start_job`]'s return values on failure, except
    ///   [`ClientError::InvalidJobState`], which indicates the job has already been started.
    /// * Forwards [`SpiderClient::get_job_state`]'s return values on failure.
    async fn run_s3_compression_job_to_completion(
        &self,
        spider_job_id: JobId,
        initial_poll_backoff: Duration,
        max_poll_backoff: Duration,
    ) -> Result<CompressionJobOutcome, Error> {
        /// The multiplier applied to the poll backoff after each non-terminal poll.
        const POLL_BACKOFF_FACTOR: u32 = 2;

        match self.start_job(spider_job_id).await {
            Ok(_) | Err(ClientError::InvalidJobState(_)) => {}
            Err(error) => return Err(error.into()),
        }

        let mut backoff = initial_poll_backoff.min(max_poll_backoff);
        let terminal_state = loop {
            let state = self.get_job_state(spider_job_id).await?;
            if state.is_terminal() {
                break state;
            }
            tokio::time::sleep(backoff).await;
            backoff = backoff
                .saturating_mul(POLL_BACKOFF_FACTOR)
                .min(max_poll_backoff);
        };

        Ok(match terminal_state {
            JobState::Succeeded => CompressionJobOutcome::Succeeded,
            JobState::Failed => CompressionJobOutcome::Failed {
                error_message: self
                    .get_job_error(spider_job_id)
                    .await
                    .unwrap_or_else(|error| format!("<failed to fetch job error: {error}>")),
            },
            JobState::Cancelled => CompressionJobOutcome::Cancelled,
            _ => unreachable!(),
        })
    }
}
