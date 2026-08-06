#!/usr/bin/env bash

# Runs a command with host CA trust configured, if any was supplied.
#
# Dockerfile RUN steps that reach the network invoke their command through this
# script. When `build.sh --with-ca-certs` passed a `ca_trust` build context, the
# mount at /run/ca-trust holds the staged bundle and the ca-trust library, and
# the command runs with the trust environment set. Otherwise the mount is the
# empty default stage and the command runs unchanged.
#
# This lives under lib_install/ because that directory is COPYed into every
# image, so the script is always present. It can't live in the mount it guards:
# the whole point is to handle that mount being empty.
#
# Usage: ca-trust-run.sh <cmd> [args...]

set -o errexit
set -o nounset
set -o pipefail

readonly CA_TRUST_MOUNT="/run/ca-trust"

if (( $# == 0 )); then
    echo >&2 "ERROR: ca-trust-run.sh requires a command to run"
    exit 2
fi

if [[ -e "${CA_TRUST_MOUNT}/container.sh" ]]; then
    export CA_TRUST_DIR="${CA_TRUST_MOUNT}"
    # container-exec.sh sources container.sh and then execs, so the trust
    # environment is in place for the command and its children.
    exec bash "${CA_TRUST_MOUNT}/container-exec.sh" "$@"
fi

exec "$@"
