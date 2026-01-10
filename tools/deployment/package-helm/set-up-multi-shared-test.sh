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

# shellcheck source=.set-up-common.sh
source "${script_dir}/.set-up-common.sh"

echo "=== Multi-node setup with shared worker nodes ==="
echo "Cluster: ${CLUSTER_NAME}"
echo "Worker nodes: ${NUM_WORKER_NODES}"
echo "Compression workers: ${COMPRESSION_WORKER_REPLICAS}"
echo "Query workers: ${QUERY_WORKER_REPLICAS}"
echo "Reducers: ${REDUCER_REPLICAS}"
echo ""

prepare_environment "${CLUSTER_NAME}"

echo "Creating kind cluster..."
generate_kind_config "${NUM_WORKER_NODES}" | kind create cluster --name "${CLUSTER_NAME}" --config=-

echo "Installing Helm chart..."
helm uninstall test --ignore-not-found
sleep 2
helm install test "${script_dir}" \
    --set "distributed=true" \
    --set "compressionWorker.replicas=${COMPRESSION_WORKER_REPLICAS}" \
    --set "queryWorker.replicas=${QUERY_WORKER_REPLICAS}" \
    --set "reducer.replicas=${REDUCER_REPLICAS}"

wait_for_cluster_ready
