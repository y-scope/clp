//! Construction of task graphs and their positional inputs for search queries.

use std::collections::HashMap;
use std::collections::hash_map::Entry;

use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::job_config::QueryOutput;
use clp_rust_utils::job_config::SearchQuery;
use spider_core::task::DataTypeDescriptor;
use spider_core::task::ExecutionPolicy;
use spider_core::task::TaskDescriptor;
use spider_core::task::TaskGraph;
use spider_core::task::TdlContext;
use spider_core::task::TerminationTaskDescriptor;
use spider_core::task::ValueTypeDescriptor;
use spider_core::types::io::TaskGraphInput;
use spider_core::types::io::TaskGraphInputBuilder;

use crate::Error;
use crate::job_handle::ArchiveMetadata;

/// Builds archive-search tasks and a final publication task for timeline queries.
///
/// For timeline queries with no archives, the graph contains a single regular
/// `query::commit_timeline` task that publishes an empty timeline.
///
/// Spider may accept cancellation while the commit task runs. Published timeline data must remain
/// hidden unless the query job reaches its successful terminal state.
///
/// # Returns
///
/// On success, a tuple containing:
///
/// * The constructed task graph.
/// * The task graph's positional input.
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
pub(super) fn build_query_task_graph(
    query_job_id: QueryJobId,
    query: &SearchQuery,
    output: &QueryOutput,
    archives_to_search: Vec<(ArchiveMetadata, ExecutionPolicy)>,
) -> Result<(TaskGraph, TaskGraphInput), Error> {
    // NOTE: Keep these names and the input order in sync with the TDL package definitions.
    const CLP_TDL_PACKAGE_NAME: &str = "clp";
    const QUERY_TASK_FUNC: &str = "query::clp_s_search";

    let commit_task = output.has_timeline().then(|| TerminationTaskDescriptor {
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
    if archives_to_search.is_empty()
        && let Some(commit_task) = commit_task
    {
        let mut graph = TaskGraph::new(None, None)?;
        graph.insert_task(TaskDescriptor {
            tdl_context: commit_task.tdl_context,
            execution_policy: commit_task.execution_policy,
            inputs: vec![],
            outputs: vec![],
            input_sources: None,
        })?;
        return Ok((graph, TaskGraphInputBuilder::new().build()));
    }
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
    use spider_core::task::ExecutionPolicy;

    use super::build_query_task_graph;
    use crate::job_handle::ArchiveMetadata;

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
            let (graph, _) = build_query_task_graph(
                42,
                &query,
                &output,
                vec![(
                    ArchiveMetadata {
                        id: "00000000-0000-0000-0000-000000000001".parse()?,
                        dataset: DatasetName::default(),
                        uncompressed_size: 1,
                        end_timestamp: 0,
                    },
                    ExecutionPolicy::default(),
                )],
            )?;
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

    #[test]
    fn empty_timeline_uses_a_regular_commit_task() -> anyhow::Result<()> {
        let query = SearchQuery {
            datasets: NonEmptyDatasetNames::try_from(vec![DatasetName::default()])?,
            query_string: NonEmptyString::from_static_str("*"),
            time_range: TimeRange::default(),
            ignore_case: false,
        };
        let output = QueryOutput::ResultsCache(ResultsCacheOutput {
            max_num_results: None,
            aggregation: Some(Aggregation::CountByTime {
                bucket_size_millisecs: PositiveI64::new(60_000).expect("60,000 is positive"),
            }),
        });
        let (graph, inputs) = build_query_task_graph(42, &query, &output, vec![])?;
        assert_eq!(graph.get_num_tasks(), 1);
        assert_eq!(graph.get_commit_task_descriptor(), None);
        assert_eq!(inputs.get_positional_inputs(), &[]);
        let commit = graph.get_task(0).expect("the graph contains one task");
        assert_eq!(commit.get_tdl_context().package, "clp");
        assert_eq!(commit.get_tdl_context().task_func, "query::commit_timeline");
        assert_eq!(commit.get_input_dep_indices(), &Vec::<usize>::new());
        assert_eq!(commit.get_output_dep_indices(), &Vec::<usize>::new());
        assert_eq!(commit.get_execution_policy().max_num_retry, 3);
        assert_eq!(commit.get_execution_policy().max_num_instances, 1);
        Ok(())
    }
}
