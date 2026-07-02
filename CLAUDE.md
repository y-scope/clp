# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

CLP (Compressed Log Processor) compresses logs (JSON and unstructured text) and enables search without decompression. The repo is a monorepo with components written in C++20, Python, Rust, and TypeScript.

## Initial setup

```shell
# Initialize submodules and download source deps
tools/scripts/deps-download/init.sh

# Install C++ core's native deps (Ubuntu Jammy)
components/core/tools/scripts/lib_install/ubuntu-jammy/install-all.sh

# Download C++ source deps into build/
task deps:core
```

The primary build tool is [Task](https://taskfile.dev/) (`task`). All task invocations run from the repo root. Python components use [uv](https://docs.astral.sh/uv/). Rust uses cargo.

## Build commands

```shell
task              # Build the full clp-package (default target)
task core         # Build C++ core (clp, clp-s, clg, clo, indexer, reducer-server, log-converter)
task rust         # Build Rust components (api-server, log-ingestor)
task webui        # Build the Node.js web UI
task clp-json-pkg-tar   # Produce releasable tar for JSON-log flavor
task clp-text-pkg-tar   # Produce releasable tar for text-log flavor
task clean        # Remove all build artifacts
task clean-core   # Remove only the C++ core build
```

The full package build requires x86_64 Ubuntu 22.04 and Docker (the package runtime uses an Ubuntu Jammy container image). The built package lands in `build/clp-package/`.

To control C++ build parallelism, set `CLP_CPP_MAX_PARALLELISM_PER_BUILD_TASK` in the environment.

### Config overrides (persisted across rebuilds, git-ignored)

- `components/package-template/src/etc/clp-config.yaml` — overrides the template config
- `components/package-template/src/etc/credentials.yaml` — overrides auto-generated credentials

## Lint commands

```shell
task lint:check          # Check all linters (C++, Python, JS, Rust, YAML, Helm)
task lint:fix            # Auto-fix where possible
task lint:check-cpp-full # C++ format + clang-tidy (full file set)
task lint:check-cpp-diff # C++ format + clang-tidy (only changed files — faster in PRs)
task lint:fix-cpp-full
task lint:fix-no-cpp     # Fix JS, Python, Rust, YAML (skip slow C++ pass)
```

Linting configs extend shared rules from `tools/yscope-dev-utils/exports/lint-configs/`. Python uses ruff + mypy (strict); C++ uses clang-format + clang-tidy; Rust uses clippy + rustfmt; JS/TS uses eslint/prettier; YAML uses yamllint.

## Test commands

```shell
# Integration tests (Python/pytest)
task tests:integration:core             # Run C++ core integration tests
task tests:integration:package          # Run package integration tests
task tests:integration:clp-py-project-imports  # Verify Python package imports

# Run a single pytest test file or test
cd integration-tests
uv run pytest tests/test_log_converter.py
uv run pytest tests/package_tests/clp_json/test_clp_json.py -k "test_name"
uv run pytest -m core    # Run only tests marked @pytest.mark.core
uv run pytest -m package # Run only tests marked @pytest.mark.package

# Rust tests (requires localstack for S3/SQS)
task tests:rust-all
```

The C++ core `unitTest` binary (Catch2) is built when `CLP_BUILD_TESTING=ON` (enabled by default when `BUILD_TESTING` is set). It's exercised via `task tests:integration:core`.

Integration tests need environment variables pointing at the build outputs:
- `CLP_BUILD_DIR` → `build/`
- `CLP_CORE_BINS_DIR` → `build/core/`
- `CLP_PACKAGE_DIR` → `build/clp-package/`

## Architecture

### Components (`components/`)

| Component | Language | Purpose |
|---|---|---|
| `core` | C++20 | Compression, decompression, search engines (clp/clp-s for text/JSON, clg/clo for grep/decompress) |
| `api-server` | Rust | REST API for query submission and results |
| `log-ingestor` | Rust | Reads logs from S3/SQS and triggers compression |
| `clp-rust-utils` | Rust | Shared Rust utilities |
| `job-orchestration` | Python | Celery-based scheduler and worker cluster |
| `clp-py-utils` | Python | Shared Python utilities (config models, DB helpers) |
| `clp-package-utils` | Python | CLI tools for operating the CLP package |
| `clp-mcp-server` | Python | MCP server exposing CLP search to AI assistants |
| `webui` | TypeScript/Node.js | Main web interface (client + server) |
| `log-viewer-webui` | TypeScript | Standalone log viewer (git submodule in webui) |
| `package-template` | — | Base directory structure for the distributable package |

The Rust workspace is defined at the repo root (`Cargo.toml`). Python components are each standalone `uv`-managed projects built into `.whl` files under `dist/`, then installed together into `build/python-libs/`.

### Runtime service architecture

The package deploys these services (via Docker Compose or Helm/Kubernetes):

- **Infrastructure**: MySQL (metadata DB), RabbitMQ (task queue), Redis (Celery backend), MongoDB (query results cache)
- **Schedulers**: `compression-scheduler`, `query-scheduler`, `spider-scheduler` — consume jobs from the DB and dispatch tasks to workers via RabbitMQ
- **Workers**: `compression-worker`, `query-worker`, `spider-compression-worker` — Celery workers that run the actual clp/clp-s/spider binaries
- **Reducer**: aggregates streaming query results
- **API server**: Rust service that accepts REST calls, writes jobs to MySQL, and reads results from MongoDB
- **WebUI**: Node.js server + Vite-built client; interfaces with the API server
- **MCP server**: exposes search to AI assistants

### Two storage/query engine flavors

- `clp-text`: uses `clp` and `clg` for unstructured text logs
- `clp-json`: uses `clp-s` for JSON logs (KV-IR format)

Both are built into the same package; the active flavor is controlled by `package.storage_engine` and `package.query_engine` in `clp-config.yaml`.

### Key design points

- Archives are the on-disk format: compressed log segments plus a metadata SQLite DB per archive, with global metadata in MySQL.
- The `reducer` component enables aggregation queries over streaming results from distributed workers.
- Spider is a third-party log ingestion engine integrated via `yscope-spider-py`.
- The webui `yscope-log-viewer` is a git submodule at `components/webui/yscope-log-viewer`; `tools/yscope-dev-utils` is another submodule at `tools/yscope-dev-utils`.
