# CLP Rust Libraries

This directory contains the Rust library crates shared by CLP's Rust components.

A crate belongs here if other crates depend on it and it has no deployable artifact of its own.
Services and other deployables stay directly under `components/`, even when they have a `[lib]`
target. For example, `clp-tdl-package` builds the shared library loaded by Spider workers, so it
stays there.

Each crate is in its own subdirectory, and new crates must be added to the workspace `members` in
the root `Cargo.toml`.

## Crates

* [clp-rust-utils]: Common utilities used by other Rust components in the CLP package.

[clp-rust-utils]: clp-rust-utils/README.md
