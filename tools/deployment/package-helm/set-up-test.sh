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
echo "Presto: ${ENABLE_PRESTO}"
echo ""

prepare_environment "${CLUSTER_NAME}"

echo "Creating kind cluster..."
generate_kind_config 0 | kind create cluster --name "${CLUSTER_NAME}" --config=-

echo "Installing Helm chart..."
helm uninstall test --ignore-not-found
sleep 2

# Resolve the local-image overrides into Helm --set flags up front so a failure
# (an invalid image ref, or an image absent from the local Docker daemon) exits
# loudly instead of being silently dropped — which would make `helm install`
# fall back to the chart-default image. An empty override is intentional (no
# --clp-*-image passed) and resolves to empty flags.
clp_package_args=$(get_image_helm_args "${CLUSTER_NAME}" "clpPackage" "${CLP_PACKAGE_IMAGE}") || exit 1
clp_connector_args=$(get_image_helm_args "${CLUSTER_NAME}" "clpConnector" "${CLP_PRESTO_CONNECTOR_IMAGE}") || exit 1

# Word splitting is intentional: helper functions return multiple --set flags.
# shellcheck disable=SC2086,SC2046
helm install test "${script_dir}" \
    $(get_service_exposure_helm_args) \
    $(get_presto_helm_args) \
    ${clp_package_args} \
    ${clp_connector_args}

wait_for_cluster_ready
