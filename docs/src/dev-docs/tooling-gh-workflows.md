# GitHub workflows

The CLP repo includes several GitHub workflows for automating container image builds, artifact
builds, testing, and linting. We briefly describe each workflow below.

## clp-artifact-build

This workflow is responsible for the following:

1. Building container images containing the dependencies necessary to build all CLP artifacts.
2. Building CLP-core, the CLP-core Python wheels, and the CLP package.
3. Building container images containing:

    * CLP-core;
    * the CLP package; and
    * the binaries necessary for the Spider worker.

4. Running C++ linting checks, unit tests, and integration tests on the built artifacts.

To minimize build times, the jobs in the workflow are organized in the directed acyclic graph (DAG)
shown below.

:::{mermaid}
%%{
  init: {
    "theme": "base",
    "themeVariables": {
      "primaryColor": "#0066cc",
      "primaryTextColor": "#fff",
      "primaryBorderColor": "transparent",
      "lineColor": "#007fff",
      "secondaryColor": "#007fff",
      "tertiaryColor": "#fff"
    }
  }
}%%
flowchart LR
    %% CLP-artifact-dependency container build jobs
    filter-relevant-changes --> centos-stream-9-deps-image
    filter-relevant-changes --> manylinux_2_28-deps-image
    filter-relevant-changes --> musllinux_1_2-deps-image
    filter-relevant-changes --> ubuntu-jammy-aarch64-deps-image
    filter-relevant-changes --> ubuntu-jammy-x86_64-deps-image
    manylinux_2_28-deps-image --> manylinux_2_28-deps-image-merge
    musllinux_1_2-deps-image --> musllinux_1_2-deps-image-merge

    %% CLP-core build jobs
    filter-relevant-changes --> centos-stream-9-binaries
    centos-stream-9-deps-image --> centos-stream-9-binaries
    filter-relevant-changes --> manylinux_2_28-x86_64-binaries
    manylinux_2_28-deps-image --> manylinux_2_28-x86_64-binaries
    manylinux_2_28-deps-image-merge --> manylinux_2_28-x86_64-binaries
    filter-relevant-changes --> musllinux_1_2-x86_64-binaries
    musllinux_1_2-deps-image --> musllinux_1_2-x86_64-binaries
    musllinux_1_2-deps-image-merge --> musllinux_1_2-x86_64-binaries
    filter-relevant-changes --> ubuntu-jammy-binaries
    ubuntu-jammy-x86_64-deps-image --> ubuntu-jammy-binaries

    %% CLP-core binaries container build jobs
    ubuntu-jammy-binaries --> ubuntu-jammy-binaries-image

    %% CLP-core Python-wheel build jobs
    filter-relevant-changes --> manylinux_2_28-x86_64-python-wheels
    manylinux_2_28-deps-image --> manylinux_2_28-x86_64-python-wheels
    manylinux_2_28-deps-image-merge --> manylinux_2_28-x86_64-python-wheels

    %% CLP-package container build jobs
    filter-relevant-changes --> package-image
    ubuntu-jammy-aarch64-deps-image --> package-image
    ubuntu-jammy-x86_64-deps-image --> package-image
    package-image --> package-image-multiarch-manifest

    %% Spider-worker container build jobs
    filter-relevant-changes --> spider-worker-image
    ubuntu-jammy-x86_64-deps-image --> spider-worker-image

    %% Lint & test jobs
    filter-relevant-changes --> ubuntu-jammy-lint
    ubuntu-jammy-x86_64-deps-image --> ubuntu-jammy-lint
    ubuntu-jammy-binaries --> ubuntu-jammy-integration-tests-core
:::

Arrows between jobs indicate a dependency. The jobs are as follows:

* `filter-relevant-changes`: Filters the changes in the pull request or commit to determine which of
  the following jobs should run.
* `centos-stream-9-deps-image`: Builds a container image containing the dependencies necessary to
  build CLP-core in a CentOS Stream 9 x86 environment.
* `manylinux_2_28-deps-image`: A matrix job that builds, for each of amd64 and arm64 natively on
  its matching runner, a container image containing the dependencies necessary to build CLP-core
  in a manylinux_2_28 environment. On push to `main`, each arch is published under an
  arch-suffixed tag (e.g. `:main-amd64`).
* `manylinux_2_28-deps-image-merge`: On push to `main`, merges the per-arch tags produced by
  `manylinux_2_28-deps-image` into a single multi-arch `:main` manifest.
* `musllinux_1_2-deps-image`: A matrix job that builds, for each of amd64 and arm64 natively on
  its matching runner, a container image containing the dependencies necessary to build CLP-core
  in a musllinux_1_2 environment. On push to `main`, each arch is published under an
  arch-suffixed tag (e.g. `:main-amd64`).
* `musllinux_1_2-deps-image-merge`: On push to `main`, merges the per-arch tags produced by
  `musllinux_1_2-deps-image` into a single multi-arch `:main` manifest.
* `ubuntu-jammy-x86_64-deps-image` / `ubuntu-jammy-aarch64-deps-image`: Builds a container image
  containing the dependencies necessary to build CLP-core in an Ubuntu Jammy x86/aarch64
  environment.
* `centos-stream-9-binaries`: Builds the CLP-core binaries in the built CentOS Stream 9 container
  and runs core's unit tests.
* `manylinux_2_28-x86_64-binaries`: Builds the CLP-core binaries in the built manylinux_2_28
  container and runs core's unit tests.
* `musllinux_1_2-x86_64-binaries`: Builds the CLP-core binaries in the built musllinux_1_2 container
  and runs core's unit tests.
* `manylinux_2_28-x86_64-python-wheels`: Builds the `yscope-clp-core` wheels in the built
  `manylinux_2_28` container.
* `ubuntu-jammy-binaries`: Builds the CLP-core binaries in the built Ubuntu Jammy container and runs
  core's unit tests.
* `ubuntu-jammy-binaries-image`: Builds an Ubuntu Jammy container image containing CLP-core's
  binaries built in the `ubuntu-jammy-binaries` job.
* `ubuntu-jammy-lint`: Runs C++ linting checks in the built ubuntu-jammy container.
* `ubuntu-jammy-integration-tests-core`: Runs CLP-core's integration tests using the binaries built
  in the `ubuntu-jammy-binaries` job, and then uploads the logs from the tests.
* `package-image`: Builds the CLP package container image.
* `package-image-multiarch-manifest`: When run on `main`, merges the per-arch tags produced by
  `package-image` into a single multi-arch manifest.
* `spider-worker-image`: Builds a container image containing CLP-core and `clp-tdl-package`.

When the PR or commit doesn't change any of the files that affect CLP's dependencies (or the
dependency container images), then the dependency container images won't be rebuilt; instead the
published images (from ghcr.io) will be used.

If a PR or commit *does* change the dependencies, then the relevant dependency image(s) will be
rebuilt, and those will be used by the dependent jobs. Specifically, if the change is from a commit,
the image(s) will be published to ghcr.io and then dependent jobs will pull the image(s) from there.
If the change is from a PR, the image(s) will be uploaded to temporary storage provided by GitHub
Actions, and then dependent jobs will download and load the image(s) from there.

Note that for the images containing CLP's dependencies (built by the `xxx-deps-image` jobs), we need
to build and test an image for each Linux distro where we support building CLP natively. However,
for the image containing CLP's binaries (built by the `ubuntu-jammy-binaries-image` job), we only
need it for one OS since users can use the container on any OS.

### Runner configuration

The workflow automatically selects runners based on the repository owner:

* For branches and pull requests on the original `y-scope`-owned repository, it uses self-hosted
  runners with tags `["self-hosted", "x64", "ubuntu-noble"]`. Our self-hosted runner pool consists
  of machines with 8–16 cores and 128–256 GB of RAM.
* For forks, it automatically falls back to GitHub-hosted runners.

Note that GitHub-hosted runners have limited resources (e.g., 4 cores, limited RAM) and limited
concurrency for free-tier organizations, so builds are expected to take longer on forks. For more
details, see [GitHub-hosted runners][gh-hosted-runners].

## clp-core-build-macos

This workflow builds CLP-core on macOS and runs its unit tests.

## clp-docs

This workflow validates that the docs site can be built.

## clp-docs-generated-code-checks

This workflow generates the OpenAPI docs and validates that they don't differ from the committed
OpenAPI docs.

## clp-lint

This workflow runs all JavaScript, Python, and YAML linting checks on the codebase.

:::{note}
C++, Rust, and Helm linting checks are run in the `clp-artifact-build`, `clp-rust-checks`, and
`clp-package-helm` workflows, respectively.
:::

## clp-package-helm

This workflow contains two jobs for linting, building, and publishing the Helm chart:

* `lint` runs Helm linting checks on the chart.
* `publish` builds the chart; then on pushes to `main` and semantic-version branches, the job
  publishes the built chart to the `gh-pages` branch.

## clp-pr-title-checks

This workflow validates pull request titles against the Conventional Commits specification.

## clp-rust-checks

This workflow validates Rust's lock files, runs all Rust linting checks, and runs all Rust unit
tests.

## clp-s-generated-code-checks

This workflow generates the KQL and SQL ANTLR parsers and validates that they don't differ from the
committed parsers.

## clp-uv-checks

This workflow checks whether each UV Python project's lockfile matches the project metadata.

[gh-hosted-runners]: https://docs.github.com/en/actions/using-github-hosted-runners/using-github-hosted-runners/about-github-hosted-runners
