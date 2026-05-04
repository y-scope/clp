#!/usr/bin/env bash

# Multi-node cluster setup with shared worker nodes
# Both compression and query workers share the same node pool
# TODO: Migrate into integration test

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

CLP_HOME="${CLP_HOME:-/tmp/clp}"
CLUSTER_NAME="${CLUSTER_NAME:-clp-test}"
NUM_WORKER_NODES="${NUM_WORKER_NODES:-2}"
COMPRESSION_WORKER_REPLICAS="${COMPRESSION_WORKER_REPLICAS:-2}"
QUERY_WORKER_REPLICAS="${QUERY_WORKER_REPLICAS:-2}"
REDUCER_REPLICAS="${REDUCER_REPLICAS:-2}"
PRESTO_WORKER_REPLICAS="${PRESTO_WORKER_REPLICAS:-0}"

# shellcheck source=.set-up-common.sh
source "${script_dir}/.set-up-common.sh"

parse_common_args "$@"

# When --presto is passed, use non-zero defaults for Presto replicas if not explicitly set.
if [[ "${ENABLE_PRESTO}" == "true" ]]; then
    [[ "${PRESTO_WORKER_REPLICAS}" -eq 0 ]] && PRESTO_WORKER_REPLICAS=2
fi

echo "=== Multi-node setup with shared worker nodes ==="
echo "Cluster: ${CLUSTER_NAME}"
echo "Worker nodes: ${NUM_WORKER_NODES}"
echo "Compression workers: ${COMPRESSION_WORKER_REPLICAS}"
echo "Query workers: ${QUERY_WORKER_REPLICAS}"
echo "Reducers: ${REDUCER_REPLICAS}"
echo "Presto workers: ${PRESTO_WORKER_REPLICAS}"
echo "Presto: ${ENABLE_PRESTO}"
echo ""

prepare_environment "${CLUSTER_NAME}"

echo "Creating kind cluster..."
generate_kind_config "${NUM_WORKER_NODES}" | kind create cluster --name "${CLUSTER_NAME}" --config=-

echo "Installing Helm chart..."
helm uninstall test --ignore-not-found
sleep 2
# Word splitting is intentional: helper functions return multiple --set flags.
# shellcheck disable=SC2046
helm install test "${script_dir}" \
    --set "distributedDeployment=true" \
    --set "scheduling.compressionWorker.replicas=${COMPRESSION_WORKER_REPLICAS}" \
    --set "scheduling.queryWorker.replicas=${QUERY_WORKER_REPLICAS}" \
    --set "scheduling.reducer.replicas=${REDUCER_REPLICAS}" \
    --set "scheduling.prestoWorker.replicas=${PRESTO_WORKER_REPLICAS}" \
    $(get_presto_helm_args) \
    $(get_image_helm_args "${CLUSTER_NAME}" "${CLP_PACKAGE_IMAGE}")

wait_for_cluster_ready
