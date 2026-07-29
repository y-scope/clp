use std::path::PathBuf;

use clap::Parser;

/// Command-line arguments for the compression coordinator.
#[derive(Debug, Parser)]
#[command(about = "Run the compression coordinator.")]
struct Cli {
    /// Path to the configuration file.
    #[arg(short, long, value_name = "PATH")]
    config: PathBuf,
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let _cli = Cli::parse();

    Ok(())
}
