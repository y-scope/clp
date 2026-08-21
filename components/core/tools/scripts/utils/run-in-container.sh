#!/usr/bin/env bash

# Runs a command inside the CLP-core dependencies image, with this component
# bind-mounted.
#
# Options:
#   --with-ca-certs  Propagate the host's CA trust into the container, for use
#                    behind a corporate TLS gateway. Off by default.
#
# Everything after the options is the command to run.

set -o errexit
set -o nounset
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
component_root="$script_dir/../../../"
# Five levels: utils -> scripts -> tools -> core -> components -> repo root.
repo_root="$script_dir/../../../../../"

with_ca_certs=0
if [[ "${1:-}" == "--with-ca-certs" ]]; then
    with_ca_certs=1
    shift
fi

# The dependencies image carries no host CA certificates -- they are never baked
# into a published image -- so a corporate TLS gateway needs the host's bundle
# mounted at run time. It matters here because commands run in this container
# (dependency downloads, package installs) reach the network.
ca_trust_args=()
ca_trust_cmd_prefix=()
if (( with_ca_certs )); then
    # shellcheck source=tools/yscope-dev-utils/exports/docker/ca-trust/host.sh
    source "${repo_root}/tools/yscope-dev-utils/exports/docker/ca-trust/host.sh"
    ca_trust_dir="$(mktemp -d)"
    trap 'rm -rf "${ca_trust_dir}"' EXIT
    ca_trust_stage_or_fail "${ca_trust_dir}"
    # Stage the library alongside the bundle: mounting the bundle alone does
    # nothing, because something inside the container has to source container.sh
    # to export the trust environment. container-exec.sh does that and then
    # execs the user's command, so it is prefixed to the command below.
    ca_trust_stage_build_context "${ca_trust_dir}"
    ca_trust_add_run_args ca_trust_args "${ca_trust_dir}"
    ca_trust_cmd_prefix=("bash" "${CA_TRUST_CONTAINER_DIR}/container-exec.sh")
fi

# Run the user's command in the container, relative to the root of this
# component
container_component_root=/mnt/clp
docker run \
  -i \
  --rm \
  -u"$(id -u):$(id -g)" \
  --mount "type=bind,src=$(readlink -f "$component_root"),dst=$container_component_root" \
  ${ca_trust_args[@]+"${ca_trust_args[@]}"} \
  -w "$container_component_root" \
  ghcr.io/y-scope/clp/clp-core-dependencies-x86-ubuntu-jammy:main \
  ${ca_trust_cmd_prefix[@]+"${ca_trust_cmd_prefix[@]}"} "$@"
