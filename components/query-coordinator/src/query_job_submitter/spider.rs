//! [`QueryJobSubmitter`] implementation for [`spider_client::SpiderClient`].

use std::time::Duration;

use async_trait::async_trait;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::task_io::query::ClpSQueryOption;
use clp_rust_utils::task_io::query::OutputHandle;
use spider_client::SpiderClient;
use spider_client::error::ClientError;
use spider_core::job::JobState;
use spider_core::task::DataTypeDescriptor;
use spider_core::task::ExecutionPolicy;
use spider_core::task::TaskDescriptor;
use spider_core::task::TaskGraph;
use spider_core::task::TdlContext;
use spider_core::task::ValueTypeDescriptor;
use spider_core::types::id::JobId;
use spider_core::types::id::ResourceGroupId;
use spider_core::types::io::TaskInput;

use crate::Error;
use crate::query_job_submitter::ArchiveMetadata;
use crate::query_job_submitter::QueryJobOutcome;
use crate::query_job_submitter::QueryJobSubmitter;

#[async_trait]
impl QueryJobSubmitter for SpiderClient {
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`build_query_task_graph`]'s return values on failure.
    /// * Forwards [`SpiderClient::submit_job`]'s return values on failure.
    async fn submit_query_job(
        &self,
        query_job_id: QueryJobId,
        resource_group_id: ResourceGroupId,
        clp_s_query_option: ClpSQueryOption,
        output_handle: OutputHandle,
        archives_to_search: Vec<(ArchiveMetadata, ExecutionPolicy)>,
    ) -> Result<JobId, Error> {
        let (graph, inputs) = build_query_task_graph(
            query_job_id,
            &clp_s_query_option,
            &output_handle,
            archives_to_search,
        )?;
        let spider_job_id = self.submit_job(resource_group_id, &graph, inputs).await?;

        tracing::info!(
            query_job_id = % query_job_id,
            spider_job_id = % spider_job_id,
            num_tasks = graph.get_num_tasks(),
            "Submitted query job to Spider.",
        );

        Ok(spider_job_id)
    }

    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`SpiderClient::start_job`]'s return values on failure, except
    ///   [`ClientError::InvalidJobState`].
    /// * Forwards [`SpiderClient::get_job_state`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if Spider returns a terminal state without a corresponding [`QueryJobOutcome`].
    async fn run_query_job_to_completion(
        &self,
        spider_job_id: JobId,
        initial_poll_backoff: Duration,
        max_poll_backoff: Duration,
    ) -> Result<QueryJobOutcome, Error> {
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
            JobState::Succeeded => QueryJobOutcome::Succeeded,
            JobState::Failed => {
                let error_message = match self.get_job_error(spider_job_id).await {
                    Ok(error_message) => error_message,
                    Err(error) => {
                        tracing::warn!(
                            spider_job_id = % spider_job_id,
                            error = % error,
                            "Failed to fetch the Spider job error.",
                        );
                        format!("<failed to fetch job error: {error}>")
                    }
                };
                QueryJobOutcome::Failed { error_message }
            }
            JobState::Cancelled => todo!("query job cancellation is not implemented"),
            _ => unreachable!("a terminal Spider state must have a terminal outcome"),
        })
    }
}

/// Builds independent archive-search tasks and their positionally ordered external inputs.
///
/// # Returns
///
/// A tuple on success, containing:
///
/// * The constructed task graph.
/// * The positionally ordered external inputs.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`TaskGraph::new`]'s return values on failure.
/// * Forwards [`ValueTypeDescriptor::struct_from_name`]'s return values on failure.
/// * Forwards [`TaskGraph::insert_task`]'s return values on failure.
/// * Forwards [`rmp_serde::to_vec`]'s return values on failure.
fn build_query_task_graph(
    query_job_id: QueryJobId,
    clp_s_query_option: &ClpSQueryOption,
    output_handle: &OutputHandle,
    archives_to_search: Vec<(ArchiveMetadata, ExecutionPolicy)>,
) -> Result<(TaskGraph, Vec<TaskInput>), Error> {
    // NOTE: Keep these names and the input order in sync with the TDL package definitions.
    const CLP_TDL_PACKAGE_NAME: &str = "clp";
    const QUERY_TASK_FUNC: &str = "query::clp_s_search";

    let mut graph = TaskGraph::new(None, None)?;

    let mut inputs = Vec::new();
    for (archive, execution_policy) in archives_to_search {
        graph.insert_task(TaskDescriptor {
            tdl_context: TdlContext {
                package: CLP_TDL_PACKAGE_NAME.to_owned(),
                task_func: QUERY_TASK_FUNC.to_owned(),
            },
            execution_policy: Some(execution_policy),
            inputs: vec![
                DataTypeDescriptor::Value(ValueTypeDescriptor::int32()),
                DataTypeDescriptor::Value(ValueTypeDescriptor::struct_from_name(
                    "ClpSQueryOption",
                )?),
                DataTypeDescriptor::Value(ValueTypeDescriptor::struct_from_name(
                    "Option<NonEmptyString>",
                )?),
                DataTypeDescriptor::Value(ValueTypeDescriptor::struct_from_name("NonEmptyString")?),
                DataTypeDescriptor::Value(ValueTypeDescriptor::struct_from_name("OutputHandle")?),
            ],
            outputs: vec![],
            input_sources: None,
        })?;
        inputs.push(TaskInput::ValuePayload(rmp_serde::to_vec(&query_job_id)?));
        inputs.push(TaskInput::ValuePayload(rmp_serde::to_vec(
            clp_s_query_option,
        )?));
        inputs.push(TaskInput::ValuePayload(rmp_serde::to_vec(
            &archive.dataset,
        )?));
        inputs.push(TaskInput::ValuePayload(rmp_serde::to_vec(&archive.id)?));
        inputs.push(TaskInput::ValuePayload(rmp_serde::to_vec(output_handle)?));
    }

    Ok((graph, inputs))
}
