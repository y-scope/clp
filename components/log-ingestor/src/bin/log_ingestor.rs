use anyhow::Context;
use clap::Parser;
use clp_rust_utils::{clp_config::package, serde::yaml};
use log_ingestor::{ingestion_job_manager::IngestionJobManagerState, routes::create_router};

#[derive(Parser)]
#[command(version, about = "log-ingestor for CLP.")]
struct Args {
    /// Path to the CLP config file.
    #[arg(long)]
    config: String,

    /// Host to bind the server to.
    #[arg(long)]
    host: String,

    /// Port to bind the server to.
    #[arg(long)]
    port: u16,
}

fn read_config_and_credentials(
    args: &Args,
) -> anyhow::Result<(package::config::Config, package::credentials::Credentials)> {
    let config_path = std::path::Path::new(args.config.as_str());
    let config: package::config::Config = yaml::from_path(config_path).context(format!(
        "Failed to load config file {}",
        config_path.display()
    ))?;

    let credentials = package::credentials::Credentials {
        database: package::credentials::Database {
            password: secrecy::SecretString::new(
                std::env::var("CLP_DB_PASS")
                    .context("Expect `CLP_DB_PASS` env variable")?
                    .into_boxed_str(),
            ),
            user: std::env::var("CLP_DB_USER").context("Expect `CLP_DB_USER` env variable")?,
        },
    };
    Ok((config, credentials))
}

async fn shutdown_signal() {
    let mut sigterm = tokio::signal::unix::signal(tokio::signal::unix::SignalKind::terminate())
        .expect("failed to listen for SIGTERM");
    tokio::select! {
        _ = sigterm.recv() => {
        }
        _ = tokio::signal::ctrl_c()=> {
        }
    }
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let args = Args::parse();

    let (config, credentials) = read_config_and_credentials(&args)?;
    let _guard = clp_rust_utils::logging::set_up_logging("log_ingestor.log");

    let telemetry_id = std::env::var("CLP_INSTANCE_ID").unwrap_or_else(|_| "unknown".to_owned());
    let clp_version = std::env::var("CLP_VERSION").unwrap_or_else(|_| "unknown".to_owned());
    let deployment_method = std::env::var("CLP_DEPLOYMENT_METHOD").unwrap_or_else(|_| "unknown".to_owned());
    let os = std::env::var("CLP_HOST_OS").unwrap_or_else(|_| std::env::consts::OS.to_owned());
    let os_version = std::env::var("CLP_HOST_OS_VERSION").unwrap_or_else(|_| "unknown".to_owned());
    let arch = std::env::var("CLP_HOST_ARCH").unwrap_or_else(|_| std::env::consts::ARCH.to_owned());
    let storage_engine = std::env::var("CLP_STORAGE_ENGINE").unwrap_or_else(|_| "clp".to_owned());

    let resource = opentelemetry_sdk::Resource::new(vec![
        opentelemetry::KeyValue::new("service.name", "log-ingestor"),
        opentelemetry::KeyValue::new("telemetry.id", telemetry_id),
        opentelemetry::KeyValue::new("clp.version", clp_version),
        opentelemetry::KeyValue::new("deployment.method", deployment_method),
        opentelemetry::KeyValue::new("os.type", os),
        opentelemetry::KeyValue::new("os.version", os_version),
        opentelemetry::KeyValue::new("host.arch", arch),
        opentelemetry::KeyValue::new("clp.storage_engine", storage_engine),
    ]);

    // Init Global OpenTelemetry context via OTLP via tonic
    let provider_result = opentelemetry_otlp::new_pipeline()
        .metrics(opentelemetry_sdk::runtime::Tokio)
        .with_exporter(opentelemetry_otlp::new_exporter().tonic())
        .with_resource(resource)
        .build();
    if let Ok(provider) = provider_result {
        opentelemetry::global::set_meter_provider(provider);
    }

    let addr = format!("{}:{}", args.host, args.port);
    let listener = tokio::net::TcpListener::bind(&addr)
        .await
        .context(format!("Cannot listen to {addr}"))?;

    let log_ingestor_manager_state = IngestionJobManagerState::from_config(config, credentials)
        .await
        .inspect_err(|err| {
            tracing::error!(err = ? err, "Failed to create ingestion job manager from CLP config.");
        })?;

    let log_ingestor_router = create_router()
        .inspect_err(|err| {
            tracing::error!(err = ? err, "Failed to create router.");
        })?
        .with_state(log_ingestor_manager_state);
    tracing::info!("Server started at {addr}");
    axum::serve(listener, log_ingestor_router)
        .with_graceful_shutdown(shutdown_signal())
        .await?;
    Ok(())
}
