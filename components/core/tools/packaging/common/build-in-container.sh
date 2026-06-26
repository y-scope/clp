#!/usr/bin/env bash

# Builds CLP core binaries and packages them inside the Docker container.
# Called by build.sh via docker run.
#
# Required environment variables:
#   FORMAT_DIR   Name of the format-specific packaging directory
#   HOST_UID     UID to restore file ownership after build
#   HOST_GID     GID to restore file ownership after build
#
# Optional environment variables:
#   CORES        Number of parallel build jobs (default: computed based on CPU
#                count and memory limits, min 2 GB per core)

set -o errexit
set -o nounset
set -o pipefail

git config --global --add safe.directory "*"

# If CORES is set, pass it through; otherwise let the taskfile compute the
# optimal parallelism based on the container's actual CPU and memory limits.
export CLP_CPP_MAX_PARALLELISM_PER_BUILD_TASK="${CORES:-}"

echo "==> Building dependencies..."
task deps:core

echo "==> Building core binaries..."
task core

# BIN_DIR must match the CMake binary output directory (task core builds into
# /clp/build/core).
echo "==> Packaging..."
BIN_DIR=/clp/build/core \
OUTPUT_DIR=/clp/build \
    "/clp/components/core/tools/packaging/${FORMAT_DIR}/package.sh"

# Restore host ownership on mounted volume paths
chown -R "${HOST_UID}:${HOST_GID}" /clp/build /clp/.task
