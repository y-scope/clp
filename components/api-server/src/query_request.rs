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
