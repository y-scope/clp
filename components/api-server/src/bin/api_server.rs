use anyhow::Context;
use clap::Parser;
use clp_rust_utils::aws::AWS_DEFAULT_REGION;
use clp_rust_utils::clp_config::package;
use clp_rust_utils::clp_config::package::config::StreamOutputStorage;
use clp_rust_utils::database::mysql::create_clp_db_mysql_pool;
use clp_rust_utils::serde::yaml;

#[derive(Parser)]
#[command(version, about = "API Server for CLP.")]
struct Args {
    #[arg(long)]
    config: String,

    #[arg(long)]
    host: Option<String>,

    #[arg(long)]
    port: Option<u16>,
}

fn read_config_and_credentials(
    args: &Args,
) -> anyhow::Result<(package::config::Config, package::credentials::Credentials)> {
    let config_path = std::path::Path::new(args.config.as_str());
    let config: package::config::Config = yaml::from_path(config_path)
        .context(format!("cannot load config file {}", config_path.display()))?;

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
    let _guard = clp_rust_utils::logging::set_up_logging("api_server.log");

    let _tel_guard = clp_rust_utils::telemetry::init_telemetry(&config.telemetry)?;

    let meter = opentelemetry::global::meter("api-server");
    let startup_counter = meter.u64_counter("clp.service.event").build();

    let api_server_config = config
        .api_server
        .as_ref()
        .expect("api_server configuration is missing");
    let addr = format!(
        "{}:{}",
        args.host.unwrap_or_else(|| api_server_config.host.clone()),
        args.port.unwrap_or(api_server_config.port)
    );
    let listener = tokio::net::TcpListener::bind(&addr)
        .await
        .context(format!("Cannot listen to {addr}"))?;

    let sql_pool = create_clp_db_mysql_pool(&config.database, &credentials.database, 10)
        .await
        .context("Cannot connect to MySQL")?;
    let mongo_uri = format!(
        "mongodb://{}:{}/{}?directConnection=true",
        config.results_cache.host, config.results_cache.port, config.results_cache.db_name,
    );
    let mongodb_client = mongodb::Client::with_uri_str(mongo_uri)
        .await
        .context("Cannot connect to MongoDB")?;

    let client = api_server::client::Client::new(&config, mongodb_client.clone(), sql_pool.clone());
    let stream_output_s3_client = match &config.stream_output.storage {
        StreamOutputStorage::S3 { s3_config, .. } => Some(
            clp_rust_utils::s3::create_new_client(
                s3_config
                    .region_code
                    .as_ref()
                    .map_or(AWS_DEFAULT_REGION, non_empty_string::NonEmptyString::as_str),
                s3_config.endpoint_url.as_ref(),
                &s3_config.aws_authentication,
            )
            .await,
        ),
        StreamOutputStorage::Fs { .. } => None,
    };
    let webui_client = api_server::webui_client::WebuiClient::new(
        &config,
        mongodb_client,
        sql_pool,
        stream_output_s3_client,
    );

    let router = api_server::routes::from_app_state(api_server::routes::AppState {
        client,
        webui_client,
    })?;
    startup_counter.add(1, &[opentelemetry::KeyValue::new("type", "start")]);

    tracing::info!("Server started at {addr}");
    axum::serve(listener, router)
        .with_graceful_shutdown(shutdown_signal())
        .await?;
    Ok(())
}
