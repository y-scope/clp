#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

# This command downloads and installs the Rust toolchain using the latest stable version. If rustup
# is already installed, it updates the Rust toolchain to the latest stable version.
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y

# Source the cargo environment to make rustup and cargo available in the current shell session.
. "$HOME/.cargo/env"

# This command installs or updates the Rust nightly toolchain to the latest version with clippy and
# rustfmt components.
# NOTE: The `--allow-downgrade` flag allows rustup to select a previous version of that toolchain if
# the newest one does not support all the requested components.
rustup toolchain install nightly --allow-downgrade \
    --component clippy \
    --component rustfmt
