# GitHub Workflows

The CLP repo includes several GitHub workflows for automating container image builds, artifact
builds, testing, and linting. We briefly describe each workflow below.

## [clp-core-build](../.github/workflows/clp-core-build.yaml)

This is the main workflow for building (Linux) container images containing CLP-core's dependencies
in addition to building CLP-core and unit testing it. To minimize build times, the jobs in the
workflow are organized in the directed acyclic graph (DAG) shown below.

```mermaid
flowchart LR
    filter-relevant-changes --> centos74-deps-image
    filter-relevant-changes --> ubuntu-focal-deps-image
    filter-relevant-changes --> ubuntu-jammy-deps-image
    filter-relevant-changes --> centos74-binaries
    filter-relevant-changes --> ubuntu-focal-binaries
    filter-relevant-changes --> ubuntu-jammy-binaries
    centos74-deps-image --> centos74-binaries
    ubuntu-focal-deps-image --> ubuntu-focal-binaries
    ubuntu-jammy-deps-image --> ubuntu-jammy-binaries
    ubuntu-focal-binaries --> ubuntu-focal-binaries-image
```

Arrows between jobs indicate a dependency. The jobs are as follows:

* `filter-relevant-changes`: This filters the changes in the pull request or commit to determine
  which of the following jobs should run.
* `centos74-deps-image`: This builds a container image containing the dependencies necessary to
  build CLP-core in a CentOS 7.4 x86 environment.
* `ubuntu-focal-deps-image`: This builds a container image containing the dependencies necessary to
  build CLP-core in an Ubuntu Focal x86 environment.
* `ubuntu-jammy-deps-image`: This builds a container image containing the dependencies necessary to
  build CLP-core in an Ubuntu Jammy x86 environment.
* `centos74-binaries`: This builds the CLP-core binaries in the built CentOS 7.4 container and runs
  core's unit tests.
* `ubuntu-focal-binaries`: This builds the CLP-core binaries in the built Ubuntu Focal container and
  runs core's unit tests.
* `ubuntu-jammy-binaries`: This builds the CLP-core binaries in the built Ubuntu Jammy container and
  runs core's unit tests.
* `ubuntu-focal-binaries-image`: This builds an Ubuntu Focal container image containing CLP-core's
  binaries built in the `ubuntu-focal-binaries` job.

When the PR or commit doesn't change any of the files that affect CLP's dependencies (or the
dependency container images), then the dependency container images won't be rebuilt; instead the
published images will be used. If a PR or commit *does* change the dependencies, then the relevant
dependency image(s) will be rebuilt, and those will be used by the dependent jobs. If the change is
from a PR, the images won't be published, but will be shared between jobs by uploading and
downloading from GitHub Actions artifacts.

## [clp-core-build-macos](../.github/workflows/clp-core-build-macos.yaml)

This workflow builds CLP-core on macOS and runs its unit tests.

## [clp-execution-image-build](../.github/workflows/clp-execution-image-build.yaml)

This workflow builds a container image that contains the dependencies necessary to run the CLP
package.

## [clp-lint](../.github/workflows/clp-lint.yaml)

This workflow runs linting checks on the codebase.
