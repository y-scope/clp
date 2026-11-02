//! [`CompressionJobSubmitter`] implementation for [`spider_client::SpiderClient`].

use std::time::Duration;

use async_trait::async_trait;
use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::job_config::CompressionJobId;
use clp_rust_utils::task_io::compression::ClpSCompressionOption;
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

use crate::compression_job_submitter::CompressionJobOutcome;
use crate::compression_job_submitter::CompressionJobSubmitter;
use crate::compression_job_submitter::CompressionManifest;
use crate::error::Error;

#[async_trait]
impl CompressionJobSubmitter for SpiderClient {
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`build_compression_task_graph`]'s return values on failure.
    /// * Forwards [`SpiderClient::submit_job`]'s return values on failure.
    async fn submit_compression_job<ManifestType: CompressionManifest>(
        &self,
        compression_job_id: CompressionJobId,
        resource_group_id: ResourceGroupId,
        clp_s_option: ClpSCompressionOption,
        dataset: DatasetName,
        input_sources: Vec<(ManifestType, ExecutionPolicy)>,
        commit_task_execution_policy: ExecutionPolicy,
    ) -> Result<JobId, Error> {
        let (graph, inputs) = build_compression_task_graph(
            &clp_s_option,
            &dataset,
            input_sources,
            commit_task_execution_policy,
        )?;

        let job_id = self.submit_job(resource_group_id, &graph, &inputs).await?;

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
    async fn run_compression_job_to_completion(
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

/// Builds compression tasks and their positionally ordered external inputs.
///
/// # Type Parameters
///
/// * `ManifestType` - The concrete input manifest consumed by each compression task.
///
/// # Returns
///
/// The task graph and its external inputs on success.
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
fn build_compression_task_graph<ManifestType: CompressionManifest>(
    clp_s_option: &ClpSCompressionOption,
    dataset: &DatasetName,
    input_sources: Vec<(ManifestType, ExecutionPolicy)>,
    commit_task_execution_policy: ExecutionPolicy,
) -> Result<(TaskGraph, TaskGraphInput), Error> {
    const CLP_TDL_PACKAGE_NAME: &str = "clp";
    const COMMIT_TASK_FUNC: &str = "compression::commit";

    let commit_task = TerminationTaskDescriptor {
        tdl_context: TdlContext {
            package: CLP_TDL_PACKAGE_NAME.to_owned(),
            task_func: COMMIT_TASK_FUNC.to_owned(),
        },
        execution_policy: Some(commit_task_execution_policy),
    };
    let mut graph = TaskGraph::new(Some(commit_task), None)?;

    let mut inputs = TaskGraphInputBuilder::new();
    let compression_option_input = inputs.create_shared_input_payload(clp_s_option)?;
    let dataset_input = inputs.create_shared_input_payload(dataset)?;
    for (input_source, execution_policy) in input_sources {
        graph.insert_task(TaskDescriptor {
            tdl_context: TdlContext {
                package: CLP_TDL_PACKAGE_NAME.to_owned(),
                task_func: ManifestType::TASK_FUNC.to_owned(),
            },
            execution_policy: Some(execution_policy),
            inputs: vec![
                DataTypeDescriptor::Value(ValueTypeDescriptor::struct_from_name(
                    "ClpSCompressionOption",
                )?),
                DataTypeDescriptor::Value(ValueTypeDescriptor::struct_from_name("DatasetName")?),
                DataTypeDescriptor::Value(ValueTypeDescriptor::struct_from_name(
                    ManifestType::TYPE_NAME,
                )?),
            ],
            outputs: vec![DataTypeDescriptor::Value(
                ValueTypeDescriptor::struct_from_name("CompressionTaskOutput")?,
            )],
            input_sources: None,
        })?;
        inputs.append_shared_task_input(compression_option_input)?;
        inputs.append_shared_task_input(dataset_input)?;
        inputs.append_task_input(&input_source)?;
    }

    Ok((graph, inputs.build()))
}

#[cfg(test)]
mod tests {
    use std::fmt::Debug;

    use clp_rust_utils::clp_config::AwsAuthentication;
    use clp_rust_utils::task_io::compression::FsInputSource;
    use clp_rust_utils::task_io::compression::S3InputSource;
    use clp_rust_utils::types::non_empty_string::ExpectedNonEmpty;
    use non_empty_string::NonEmptyString;
    use serde::de::DeserializeOwned;
    use spider_core::task::TaskInputOutputIndex;
    use spider_core::types::io::TaskGraphInputEntry;

    use super::*;

    /// Verifies a manifest's graph descriptors, execution policies, and serialized inputs.
    ///
    /// # Type Parameters
    ///
    /// * `ManifestType` - The concrete input manifest to serialize and compare after decoding.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`build_compression_task_graph`]'s return values on failure.
    /// * Forwards [`ValueTypeDescriptor::struct_from_name`]'s return values on failure.
    /// * Forwards [`rmp_serde::from_slice`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if the graph, execution policies, or decoded inputs differ from their expected
    /// values.
    fn assert_graph<
        ManifestType: CompressionManifest + DeserializeOwned + PartialEq + Debug + Clone,
    >(
        manifest: &ManifestType,
        task_func: &str,
        type_name: &str,
    ) -> anyhow::Result<()> {
        const OPTIONS: ClpSCompressionOption = ClpSCompressionOption {
            target_encoded_size: 123,
            compression_level: 3,
            timestamp_key: None,
            unstructured: false,
        };

        let dataset = DatasetName::default();
        let policy = ExecutionPolicy {
            max_num_retry: 2,
            ..ExecutionPolicy::default()
        };
        let commit_policy = ExecutionPolicy {
            max_num_instances: 1,
            max_num_retry: 3,
            ..ExecutionPolicy::default()
        };
        let (graph, inputs) = build_compression_task_graph(
            &OPTIONS,
            &dataset,
            vec![(manifest.clone(), policy.clone())],
            commit_policy.clone(),
        )?;
        assert_eq!(graph.get_num_tasks(), 1);
        let task = graph.get_task(0).expect("compression task should exist");
        assert_eq!(
            task.get_tdl_context(),
            &TdlContext {
                package: "clp".to_owned(),
                task_func: task_func.to_owned()
            }
        );
        assert_eq!(task.get_execution_policy(), &policy);
        for (index, expected_type) in ["ClpSCompressionOption", "DatasetName", type_name]
            .into_iter()
            .enumerate()
        {
            assert_eq!(
                graph
                    .get_task_input(TaskInputOutputIndex {
                        task_idx: 0,
                        position: index
                    })
                    .expect("task input should exist")
                    .get_type_descriptor(),
                &DataTypeDescriptor::Value(ValueTypeDescriptor::struct_from_name(expected_type)?)
            );
        }
        assert_eq!(
            graph
                .get_task_output(TaskInputOutputIndex {
                    task_idx: 0,
                    position: 0
                })
                .expect("task output should exist")
                .get_type_descriptor(),
            &DataTypeDescriptor::Value(ValueTypeDescriptor::struct_from_name(
                "CompressionTaskOutput"
            )?)
        );
        let commit = graph
            .get_commit_task_descriptor()
            .expect("commit should be attached");
        assert_eq!(commit.tdl_context.task_func, "compression::commit");
        assert_eq!(commit.execution_policy, Some(commit_policy));
        let payloads: Vec<&[u8]> = inputs
            .get_positional_inputs()
            .iter()
            .map(|entry| match entry {
                TaskGraphInputEntry::ValuePayload(payload) => payload.as_slice(),
                TaskGraphInputEntry::SharedPayload(id) => inputs
                    .get_shared_input_payload(*id)
                    .expect("shared input should exist")
                    .as_slice(),
            })
            .collect();
        assert_eq!(
            rmp_serde::from_slice::<ClpSCompressionOption>(payloads[0])?,
            OPTIONS
        );
        assert_eq!(rmp_serde::from_slice::<DatasetName>(payloads[1])?, dataset);
        assert_eq!(
            &rmp_serde::from_slice::<ManifestType>(payloads[2])?,
            manifest
        );
        Ok(())
    }

    #[test]
    fn fs_graph_uses_concrete_payload_and_commit() -> anyhow::Result<()> {
        assert_graph(
            &FsInputSource {
                file_paths: vec!["/logs/a".to_owned().try_into()?],
            },
            "compression::clp_s_fs_compress",
            "FsInputSource",
        )
    }

    #[test]
    fn s3_graph_uses_concrete_payload_and_commit() -> anyhow::Result<()> {
        assert_graph(
            &S3InputSource {
                endpoint_url: None,
                region_code: None,
                bucket: NonEmptyString::from_static_str("logs"),
                aws_authentication: AwsAuthentication::Default,
                object_keys: vec!["logs/a".to_owned()],
            },
            "compression::clp_s_s3_compress",
            "S3InputSource",
        )
    }
}
