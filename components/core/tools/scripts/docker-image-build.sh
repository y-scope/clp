#!/usr/bin/env bash

# Shared helpers for the clp-env-base-*/build.sh scripts.
#
# Wraps yscope-dev-utils' docker/build and docker/ca-trust libraries: parses the
# common flags, optionally stages the host's CA trust, then assembles and runs
# the `docker build` command.
#
# Host CA trust is opt-in via --with-ca-certs, for builds behind a
# TLS-intercepting corporate gateway. It is off by default, and nothing is ever
# baked into the image: the bundle is mounted only for the RUN steps that reach
# the network, and disappears with the step. Builds without the flag -- which is
# every CI build -- use the base image's own distro trust store. See the
# `ca_trust` stage in each Dockerfile.

if [[ "${_CLP_DOCKER_IMAGE_BUILD_SH_LOADED:-}" == "1" ]]; then
    return 0
fi
readonly _CLP_DOCKER_IMAGE_BUILD_SH_LOADED=1

_clp_repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." &>/dev/null && pwd)"
readonly _clp_repo_root

_clp_dev_utils_dir="${_clp_repo_root}/tools/yscope-dev-utils"
if [[ ! -d "${_clp_dev_utils_dir}/exports/docker" ]]; then
    echo >&2 "ERROR: yscope-dev-utils submodule is missing or out of date."
    echo >&2 "  Run: git submodule update --init --recursive tools/yscope-dev-utils"
    return 1
fi

# shellcheck source=tools/yscope-dev-utils/exports/docker/build/host.sh
source "${_clp_dev_utils_dir}/exports/docker/build/host.sh"
# shellcheck source=tools/yscope-dev-utils/exports/docker/ca-trust/host.sh
source "${_clp_dev_utils_dir}/exports/docker/ca-trust/host.sh"

# Parses the flags common to every clp-env-base build script.
#
# Sets CLP_WITH_CA_CERTS to 1 when --with-ca-certs is passed, 0 otherwise.
#
# Args: [script args...]
parse_build_args() {
    CLP_WITH_CA_CERTS=0
    while (( $# > 0 )); do
        case "$1" in
            --with-ca-certs)
                CLP_WITH_CA_CERTS=1
                shift
                ;;
            --help)
                cat <<'EOF'
Usage: ./build.sh [--with-ca-certs]

Options:
  --with-ca-certs  Propagate the host's CA trust into the networked build steps,
                   for builds behind a corporate TLS gateway. Uses SSL_CERT_FILE
                   when set, else searches common CA-bundle locations. Nothing is
                   baked into the image. Off by default.
  --help           Show this help

Optional environment variables:
  HTTP_PROXY / HTTPS_PROXY / NO_PROXY / ALL_PROXY
                   Forwarded into the build container.
  DOCKER_NETWORK   Override the Docker network mode (auto: host for loopback proxies).
  DOCKER_PULL=false
                   Skip pulling the latest base image from the registry.
EOF
                exit 0
                ;;
            *)
                echo >&2 "ERROR: unknown option: $1"
                exit 1
                ;;
        esac
    done
}

# Stages CA trust when opted in, then finalizes and runs the build.
#
# The staging directory is created under the caller's EXIT trap, so it is
# removed whether the build succeeds or fails.
#
# Args: <cmd-array-name> <script-dir> [mirror-var-name...]
run_image_build() {
    local cmd_name="$1" script_dir="$2"
    shift 2

    if (( ! ${CLP_WITH_CA_CERTS:-0} )); then
        docker_build_finalize "${cmd_name}" "${script_dir}" "$@"
        return
    fi

    local ca_trust_dir
    ca_trust_dir="$(mktemp -d)" || return 1

    # Everything below runs in a subshell that owns the cleanup trap. A RETURN
    # trap wouldn't fire at all -- callers invoke this as a bare command under
    # `errexit`, so a failed build terminates the shell without the function
    # returning -- and setting an EXIT trap here would silently replace any
    # handler the caller had already registered. A subshell gets both: cleanup
    # on every exit path, and the caller's own traps left alone.
    (
        # shellcheck disable=SC2064  # Expand ca_trust_dir now, while it's in scope.
        trap "rm -rf '${ca_trust_dir}'" EXIT

        ca_trust_stage_or_fail "${ca_trust_dir}" || exit 1
        ca_trust_stage_build_context "${ca_trust_dir}" || exit 1
        ca_trust_add_build_args "${cmd_name}" "${ca_trust_dir}" || exit 1

        docker_build_finalize "${cmd_name}" "${script_dir}" "$@"
    )
}
