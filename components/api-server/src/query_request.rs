//! Requests for submitting search queries, and the options that a client may give for the output of
//! their results.

use clp_rust_utils::dataset::NonEmptyDatasetNames;
use clp_rust_utils::job_config::NetworkOutput;
use clp_rust_utils::job_config::QueryJobConfig;
use clp_rust_utils::job_config::QueryOutput;
use clp_rust_utils::job_config::ResultsCacheOutput;
use clp_rust_utils::job_config::SearchQuery;
use serde::Deserialize;
use utoipa::ToSchema;

/// The options that a client may give for the output of a query job's results.
///
/// The output is built from the options and from what the API server adds to them.
pub trait OutputOptions {
    /// What the API server adds to the options to build the output.
    type Internal;

    /// Builds the output from the options and what the API server adds to them.
    ///
    /// # Parameters
    ///
    /// * `internal` - What the API server adds to the options.
    ///
    /// # Returns
    ///
    /// The output of a query job's results.
    fn into_output(self, internal: Self::Internal) -> QueryOutput;
}

impl OutputOptions for ResultsCacheOutput {
    type Internal = ();

    fn into_output(self, (): Self::Internal) -> QueryOutput {
        QueryOutput::ResultsCache(self)
    }
}

/// The options of the file output, which takes none.
#[derive(Debug, Default, Deserialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct FileOutputOptions {}

impl OutputOptions for FileOutputOptions {
    type Internal = ();

    fn into_output(self, (): Self::Internal) -> QueryOutput {
        QueryOutput::File {}
    }
}

/// The options of the network output, which takes none.
#[derive(Debug, Default, Deserialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct NetworkOutputOptions {}

impl OutputOptions for NetworkOutputOptions {
    type Internal = NetworkOutput;

    fn into_output(self, internal: Self::Internal) -> QueryOutput {
        QueryOutput::Network(internal)
    }
}

/// Defines the request for submitting a search query whose results go to the output of the
/// endpoint that receives it.
///
/// # Type Parameters
///
/// * `OutputOptionsType` - The type of the options that the client may give for that output.
#[derive(Debug, Deserialize, ToSchema)]
#[serde(deny_unknown_fields)]
// Keeps the "Type Parameters" section out of the OpenAPI document.
#[schema(
    description = "Defines the request for submitting a search query whose results go to the \
                   output of the endpoint that receives it."
)]
pub struct QueryRequest<OutputOptionsType: OutputOptions> {
    /// The datasets to search within.
    pub datasets: NonEmptyDatasetNames,

    /// The search to run.
    pub query: SearchQuery,

    /// The options of the output. Omit to use the defaults.
    #[serde(default)]
    // Keeps this field's description in the OpenAPI document, which utoipa otherwise replaces with
    // the options type's, and keeps utoipa's expansion clear of `clippy::option_if_let_else`.
    #[schema(inline)]
    pub output: OutputOptionsType,
}

impl<OutputOptionsType: OutputOptions> QueryRequest<OutputOptionsType> {
    /// Converts the request into the config of a query job whose results go to the output built
    /// from the request's output options and `internal`.
    ///
    /// # Returns
    ///
    /// The query job config.
    pub fn into_query_job_config(self, internal: OutputOptionsType::Internal) -> QueryJobConfig {
        QueryJobConfig {
            datasets: self.datasets.into(),
            query: self.query,
            output: self.output.into_output(internal),
        }
    }
}

#[cfg(test)]
mod tests {
    use std::num::NonZeroU16;
    use std::num::NonZeroU32;

    use clp_rust_utils::dataset::DatasetName;
    use clp_rust_utils::job_config::TimeRange;
    use non_empty_string::NonEmptyString;

    use super::*;

    #[test]
    fn query_request_converts_to_a_query_job_config_with_the_output_of_its_options()
    -> anyhow::Result<()> {
        let datasets = vec!["logs".parse::<DatasetName>()?];
        let query = SearchQuery {
            query_string: NonEmptyString::new("level: ERROR".to_owned())
                .expect("query string is non-empty"),
            time_range: TimeRange::new(Some(1), Some(2))?,
            ignore_case: true,
        };
        let options = ResultsCacheOutput {
            max_num_results: NonZeroU32::new(7),
            aggregation: None,
        };

        let query_job_config = QueryRequest {
            datasets: NonEmptyDatasetNames::try_from(datasets.clone())?,
            query: query.clone(),
            output: options.clone(),
        }
        .into_query_job_config(());

        assert_eq!(
            query_job_config,
            QueryJobConfig {
                datasets,
                query,
                output: QueryOutput::ResultsCache(options),
            }
        );
        Ok(())
    }

    #[test]
    fn file_and_network_output_options_build_their_outputs() -> anyhow::Result<()> {
        let network_output = NetworkOutput {
            host: NonEmptyString::new("10.0.0.5".to_owned()).expect("host is non-empty"),
            port: NonZeroU16::new(3003).expect("3003 is nonzero"),
            session_token: "3f1c1a5e-5b6e-4a43-9d6b-0f2b2f5a8c11".parse()?,
        };

        assert_eq!(FileOutputOptions {}.into_output(()), QueryOutput::File {});
        assert_eq!(
            NetworkOutputOptions {}.into_output(network_output.clone()),
            QueryOutput::Network(network_output)
        );
        Ok(())
    }
}
