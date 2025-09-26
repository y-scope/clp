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

# This command installs or updates the Rust nightly toolchain to the latest version. If the nightly
# toolchain is already present, it will be updated.
rustup toolchain install nightly

# Add rustfmt and clippy components to the nightly toolchain.
rustup component add rustfmt --toolchain nightly
rustup component add clippy --toolchain nightly
