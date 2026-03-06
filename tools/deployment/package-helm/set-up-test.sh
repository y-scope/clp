#!/usr/bin/env bash

# Single-node cluster setup for testing
# TODO: Migrate into integration test

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

CLP_HOME="${CLP_HOME:-/tmp/clp}"
CLUSTER_NAME="${CLUSTER_NAME:-clp-test}"

# shellcheck source=.set-up-common.sh
source "${script_dir}/.set-up-common.sh"

parse_common_args "$@"

echo "=== Single-node setup ==="
echo "Cluster: ${CLUSTER_NAME}"
echo ""

prepare_environment "${CLUSTER_NAME}"

echo "Creating kind cluster..."
generate_kind_config 0 | kind create cluster --name "${CLUSTER_NAME}" --config=-

echo "Installing Helm chart..."
helm uninstall test --ignore-not-found
sleep 2
# Word splitting is intentional: get_image_helm_args returns multiple --set flags.
# shellcheck disable=SC2046
helm install test "${script_dir}" \
    $(get_image_helm_args "${CLUSTER_NAME}" "${CLP_PACKAGE_IMAGE}")

wait_for_cluster_ready
