//! Routes that list the logs input's directories for the `WebUI`'s path picker.

use axum::Json;
use axum::extract::Query;
use axum::extract::State;
use clp_rust_utils::logs_input::LogsInputPath;
use serde::Deserialize;
use utoipa::IntoParams;
use utoipa_axum::router::OpenApiRouter;
use utoipa_axum::routes;

use crate::routes::AppState;
use crate::routes::HandlerError;
use crate::routes::webui::log_unexpected_error;
use crate::webui_client::DirEntry;
use crate::webui_client::WebuiClient;

/// Factory function.
///
/// # Returns
///
/// A newly created [`OpenApiRouter`] with all logs-input routes registered.
pub(super) fn router() -> OpenApiRouter<AppState> {
    OpenApiRouter::new().routes(routes!(list_logs_input_entries))
}

/// Query parameters for the logs-input listing endpoint.
#[derive(Deserialize, IntoParams)]
#[into_params(parameter_in = Query)]
#[serde(deny_unknown_fields)]
struct ListParams {
    /// The logs-input path of the directory to list: relative to the logs-input mount point, with
    /// a leading `/` that denotes the mount point itself.
    #[param(value_type = String)]
    path: LogsInputPath,
}

#[utoipa::path(
    get,
    path = "/os/ls",
    description = "Lists the entries of the directory at `path` in the logs input. `path` and the \
        returned entries' `parent_path` are relative to the logs-input mount point, with a leading \
        `/` that denotes the mount point itself; `parent_path` is `path` lexically normalized. \
        Returns an empty list if `path` isn't a directory. Entries whose names aren't UTF-8 or \
        contain a line break are omitted, since no logs-input path can address them. Only the \
        clp-s storage engine with filesystem logs input is supported.",
    params(ListParams),
    responses(
        (
            status = OK,
            body = Vec<DirEntry>,
            description = "The directory's entries, in no particular order, or an empty list if \
                `path` isn't a directory; entries whose names aren't UTF-8 or contain a line break \
                are omitted"
        ),
        (
            status = BAD_REQUEST,
            description = "The query string is malformed (e.g., `path` is repeated); `path` is \
                missing, doesn't start with `/`, has a `..` component, or contains a NUL \
                character or a line break; the query has an unknown parameter; the package's \
                storage engine isn't clp-s or its logs input isn't on the filesystem; or `path` \
                resolves to outside the logs-input mount point through a symlink"
        ),
        (
            status = NOT_FOUND,
            description = "`path` doesn't exist, or has a component that is too long"
        ),
        (
            status = INTERNAL_SERVER_ERROR,
            description = "The logs-input mount point can't be resolved, or `path` can't be \
                resolved or read (e.g., it's a symlink loop, or due to a permission error)"
        )
    )
)]
async fn list_logs_input_entries(
    State(client): State<WebuiClient>,
    Query(params): Query<ListParams>,
) -> Result<Json<Vec<DirEntry>>, HandlerError> {
    let entries = client
        .list_logs_input_entries(params.path)
        .await
        .inspect_err(|err| log_unexpected_error("list_logs_input_entries", err))?;
    Ok(Json(entries))
}
