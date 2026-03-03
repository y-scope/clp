#!/usr/bin/env bash

# Multi-node cluster setup with dedicated worker nodes for each worker type
# Demonstrates nodeSelector scheduling with separate node pools
# TODO: Migrate into integration test

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

CLP_HOME="${CLP_HOME:-/tmp/clp}"
CLUSTER_NAME="${CLUSTER_NAME:-clp-test}"
NUM_COMPRESSION_NODES="${NUM_COMPRESSION_NODES:-2}"
NUM_QUERY_NODES="${NUM_QUERY_NODES:-2}"
COMPRESSION_WORKER_REPLICAS="${COMPRESSION_WORKER_REPLICAS:-2}"
QUERY_WORKER_REPLICAS="${QUERY_WORKER_REPLICAS:-2}"
REDUCER_REPLICAS="${REDUCER_REPLICAS:-2}"

# shellcheck source=.set-up-common.sh
source "${script_dir}/.set-up-common.sh"

parse_common_args "$@"

echo "=== Multi-node setup with dedicated worker nodes ==="
echo "Cluster: ${CLUSTER_NAME}"
echo "Compression nodes: ${NUM_COMPRESSION_NODES}"
echo "Query nodes: ${NUM_QUERY_NODES}"
echo "Compression workers: ${COMPRESSION_WORKER_REPLICAS}"
echo "Query workers: ${QUERY_WORKER_REPLICAS}"
echo "Reducers: ${REDUCER_REPLICAS}"
echo ""

prepare_environment "${CLUSTER_NAME}"

total_workers=$((NUM_COMPRESSION_NODES + NUM_QUERY_NODES))

echo "Creating kind cluster..."
generate_kind_config "${total_workers}" | kind create cluster --name "${CLUSTER_NAME}" --config=-

echo "Labeling worker nodes..."
mapfile -t worker_nodes < <(kubectl get nodes --selector='!node-role.kubernetes.io/control-plane' -o jsonpath='{.items[*].metadata.name}' | tr ' ' '\n')

# Label compression nodes
for ((i = 0; i < NUM_COMPRESSION_NODES; i++)); do
    echo "Labeling ${worker_nodes[$i]} as compression node"
    kubectl label node "${worker_nodes[$i]}" yscope.io/nodeType=compression --overwrite
done

# Label query nodes
for ((i = NUM_COMPRESSION_NODES; i < total_workers; i++)); do
    echo "Labeling ${worker_nodes[$i]} as query node"
    kubectl label node "${worker_nodes[$i]}" yscope.io/nodeType=query --overwrite
done

echo "Installing Helm chart..."
helm uninstall test --ignore-not-found
sleep 2
# Word splitting is intentional: get_image_helm_args returns multiple --set flags.
# shellcheck disable=SC2046
helm install test "${script_dir}" \
    --set "distributedDeployment=true" \
    --set "compressionWorker.replicas=${COMPRESSION_WORKER_REPLICAS}" \
    --set "compressionWorker.scheduling.nodeSelector.yscope\.io/nodeType=compression" \
    --set "queryWorker.replicas=${QUERY_WORKER_REPLICAS}" \
    --set "queryWorker.scheduling.nodeSelector.yscope\.io/nodeType=query" \
    --set "reducer.replicas=${REDUCER_REPLICAS}" \
    $(get_image_helm_args "${CLUSTER_NAME}" "${CLP_PACKAGE_IMAGE}")

wait_for_cluster_ready
