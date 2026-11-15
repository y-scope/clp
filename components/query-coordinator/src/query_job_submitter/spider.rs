//! [`QueryJobSubmitter`] implementation for [`spider_client::SpiderClient`].

use std::collections::HashMap;
use std::collections::hash_map::Entry;
use std::time::Duration;

use async_trait::async_trait;
use clp_rust_utils::job_config::Aggregation;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::job_config::QueryOutput;
use clp_rust_utils::job_config::ResultsCacheOutput;
use clp_rust_utils::job_config::SearchQuery;
use spider_client::SpiderClient;
use spider_client::error::ClientError;
use spider_core::job::JobState;
use spider_core::task::DataTypeDescriptor;
use spider_core::task::ExecutionPolicy;
use spider_core::task::TaskDescriptor;
use spider_core::task::TaskGraph;
use spider_core::task::TdlContext;
use spider_core::task::TerminationTaskDescriptor;
use spider_core::task::ValueTypeDescriptor;
use spider_core::types::id::JobId;
use spider_core::types::id::ResourceGroupId;
use spider_core::types::io::TaskGraphInput;
use spider_core::types::io::TaskGraphInputBuilder;

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
        query: SearchQuery,
        output: QueryOutput,
        archives_to_search: Vec<(ArchiveMetadata, ExecutionPolicy)>,
    ) -> Result<JobId, Error> {
        let (graph, inputs) =
            build_query_task_graph(query_job_id, &query, &output, archives_to_search)?;
        let spider_job_id = self.submit_job(resource_group_id, &graph, &inputs).await?;

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
    ///   [`ClientError::InvalidJobState`], which indicates the job has already been started.
    /// * Forwards [`SpiderClient::get_job_state`]'s return values on failure.
    async fn run_query_job_to_completion(
        &self,
        spider_job_id: JobId,
        poll_interval: Duration,
    ) -> Result<QueryJobOutcome, Error> {
        match self.start_job(spider_job_id).await {
            Ok(_) | Err(ClientError::InvalidJobState(_)) => {}
            Err(error) => return Err(error.into()),
        }

        let terminal_state = loop {
            let state = self.get_job_state(spider_job_id).await?;
            if state.is_terminal() {
                break state;
            }
            tokio::time::sleep(poll_interval).await;
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
            JobState::Cancelled => QueryJobOutcome::Cancelled,
            _ => unreachable!("a terminal Spider state must have a terminal outcome"),
        })
    }

    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`SpiderClient::cancel_job`]'s return values on failure, except
    ///   [`ClientError::InvalidJobState`], which indicates the job is already being cancelled or
    ///   has already terminated.
    async fn cancel_query_job(&self, spider_job_id: JobId) -> Result<(), Error> {
        match self.cancel_job(spider_job_id).await {
            Ok(_) | Err(ClientError::InvalidJobState(_)) => Ok(()),
            Err(error) => Err(error.into()),
        }
    }
}

/// Builds archive-search tasks and a final publication task for timeline queries.
///
/// Spider may accept cancellation while the commit task runs. Published timeline data must remain
/// hidden unless the query job reaches its successful terminal state.
///
/// # Returns
///
/// A tuple on success, containing:
///
/// * The constructed task graph.
/// * The structured form of the task graph's input.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`TaskGraph::new`]'s return values on failure.
/// * Forwards [`ValueTypeDescriptor::struct_from_name`]'s return values on failure.
/// * Forwards [`TaskGraph::insert_task`]'s return values on failure.
/// * Forwards [`TaskGraphInputBuilder::create_shared_input_payload`]'s return values on failure.
/// * Forwards [`TaskGraphInputBuilder::append_shared_task_input`]'s return values on failure.
/// * Forwards [`TaskGraphInputBuilder::append_task_input`]'s return values on failure.
fn build_query_task_graph(
    query_job_id: QueryJobId,
    query: &SearchQuery,
    output: &QueryOutput,
    archives_to_search: Vec<(ArchiveMetadata, ExecutionPolicy)>,
) -> Result<(TaskGraph, TaskGraphInput), Error> {
    // NOTE: Keep these names and the input order in sync with the TDL package definitions.
    const CLP_TDL_PACKAGE_NAME: &str = "clp";
    const QUERY_TASK_FUNC: &str = "query::clp_s_search";

    let commit_task = matches!(
        output,
        QueryOutput::ResultsCache(ResultsCacheOutput {
            aggregation: Some(Aggregation::CountByTime { .. }),
            ..
        })
    )
    .then(|| TerminationTaskDescriptor {
        tdl_context: TdlContext {
            package: CLP_TDL_PACKAGE_NAME.to_owned(),
            task_func: "query::commit_timeline".to_owned(),
        },
        execution_policy: Some(ExecutionPolicy {
            max_num_retry: 3,
            max_num_instances: 1,
            ..ExecutionPolicy::default()
        }),
    });
    let mut graph = TaskGraph::new(commit_task, None)?;

    let mut inputs = TaskGraphInputBuilder::new();
    let query_job_id_input = inputs.create_shared_input_payload(&query_job_id)?;
    let query_input = inputs.create_shared_input_payload(query)?;
    let output_input = inputs.create_shared_input_payload(output)?;
    let mut dataset_inputs = HashMap::new();
    for (archive, execution_policy) in archives_to_search {
        graph.insert_task(TaskDescriptor {
            tdl_context: TdlContext {
                package: CLP_TDL_PACKAGE_NAME.to_owned(),
                task_func: QUERY_TASK_FUNC.to_owned(),
            },
            execution_policy: Some(execution_policy),
            inputs: vec![
                DataTypeDescriptor::Value(ValueTypeDescriptor::int32()),
                DataTypeDescriptor::Value(ValueTypeDescriptor::struct_from_name("SearchQuery")?),
                DataTypeDescriptor::Value(ValueTypeDescriptor::struct_from_name("DatasetName")?),
                DataTypeDescriptor::Value(ValueTypeDescriptor::struct_from_name("NonEmptyString")?),
                DataTypeDescriptor::Value(ValueTypeDescriptor::struct_from_name("QueryOutput")?),
            ],
            outputs: vec![],
            input_sources: None,
        })?;
        inputs.append_shared_task_input(query_job_id_input)?;
        inputs.append_shared_task_input(query_input)?;
        let dataset_input = match dataset_inputs.entry(archive.dataset) {
            Entry::Occupied(entry) => *entry.get(),
            Entry::Vacant(entry) => {
                let input = inputs.create_shared_input_payload(entry.key())?;
                *entry.insert(input)
            }
        };
        inputs.append_shared_task_input(dataset_input)?;
        inputs.append_task_input(&archive.id)?;
        inputs.append_shared_task_input(output_input)?;
    }

    Ok((graph, inputs.build()))
}

#[cfg(test)]
mod tests {
    use std::num::NonZeroU16;

    use clp_rust_utils::dataset::DatasetName;
    use clp_rust_utils::dataset::NonEmptyDatasetNames;
    use clp_rust_utils::job_config::Aggregation;
    use clp_rust_utils::job_config::NetworkOutput;
    use clp_rust_utils::job_config::QueryOutput;
    use clp_rust_utils::job_config::ResultsCacheOutput;
    use clp_rust_utils::job_config::SearchQuery;
    use clp_rust_utils::job_config::TimeRange;
    use clp_rust_utils::types::PositiveI64;
    use clp_rust_utils::types::SessionToken;
    use clp_rust_utils::types::non_empty_string::ExpectedNonEmpty;
    use non_empty_string::NonEmptyString;

    use super::build_query_task_graph;

    #[test]
    fn commit_task_is_attached_only_for_count_by_time_output() -> anyhow::Result<()> {
        let query = SearchQuery {
            datasets: NonEmptyDatasetNames::try_from(vec![DatasetName::default()])?,
            query_string: NonEmptyString::from_static_str("*"),
            time_range: TimeRange::default(),
            ignore_case: false,
        };
        for (output, has_commit) in [
            (
                QueryOutput::ResultsCache(ResultsCacheOutput {
                    max_num_results: None,
                    aggregation: Some(Aggregation::CountByTime {
                        bucket_size_millisecs: PositiveI64::new(60_000)
                            .expect("60,000 is positive"),
                    }),
                }),
                true,
            ),
            (
                QueryOutput::ResultsCache(ResultsCacheOutput {
                    max_num_results: None,
                    aggregation: None,
                }),
                false,
            ),
            (
                QueryOutput::Network(NetworkOutput {
                    host: NonEmptyString::from_static_str("localhost"),
                    port: NonZeroU16::new(1234).expect("1,234 is nonzero"),
                    session_token: SessionToken::from(uuid::Uuid::nil()),
                }),
                false,
            ),
        ] {
            let (graph, _) = build_query_task_graph(42, &query, &output, vec![])?;
            let commit = graph.get_commit_task_descriptor();
            assert_eq!(commit.is_some(), has_commit);
            if let Some(commit) = commit {
                assert_eq!(commit.tdl_context.package, "clp");
                assert_eq!(commit.tdl_context.task_func, "query::commit_timeline");
                let policy = commit
                    .execution_policy
                    .as_ref()
                    .expect("commit policy is set");
                assert_eq!(policy.max_num_retry, 3);
                assert_eq!(policy.max_num_instances, 1);
            }
        }
        Ok(())
    }
}
