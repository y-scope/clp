use std::io::Write;

use anyhow::Result;
use clap::Parser;
use utoipa::OpenApi;

#[derive(Parser)]
#[command(version, about = "Generate openapi.json")]
struct Args {
    path: String,
}

fn main() -> Result<()> {
    let mut file = std::fs::File::create(Args::parse().path)?;
    let api = log_ingestor::routes::ApiDoc::openapi();
    write!(file, "{}", api.to_json()?)?;
    Ok(())
}
