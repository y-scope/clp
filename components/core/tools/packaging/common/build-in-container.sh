#!/usr/bin/env bash

# Builds CLP core binaries and packages them inside the Docker container.
# Called by build.sh via docker run.
#
# Required environment variables:
#   CORES        Number of parallel build jobs
#   FORMAT_DIR   Name of the format-specific packaging directory
#   HOST_UID     UID to restore file ownership after build
#   HOST_GID     GID to restore file ownership after build

set -o errexit
set -o nounset
set -o pipefail

git config --global --add safe.directory "*"

echo "==> Building dependencies..."
CLP_CPP_MAX_PARALLELISM_PER_BUILD_TASK="${CORES}" task deps:core

echo "==> Building core binaries..."
CLP_CPP_MAX_PARALLELISM_PER_BUILD_TASK="${CORES}" task core

# BIN_DIR must match the CMake binary output directory (task core builds into
# /clp/build/core).
echo "==> Packaging..."
BIN_DIR=/clp/build/core \
OUTPUT_DIR=/clp/build \
    "/clp/components/core/tools/packaging/${FORMAT_DIR}/package.sh"

# Restore host ownership on mounted volume paths
chown -R "${HOST_UID}:${HOST_GID}" /clp/build /clp/.task
