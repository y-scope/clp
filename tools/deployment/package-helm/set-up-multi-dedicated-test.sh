#!/usr/bin/env bash

# Multi-node cluster test with dedicated worker nodes for each worker type
# Demonstrates nodeSelector scheduling with separate node pools
#
# To clean up after running:
#   kind delete cluster --name "${CLUSTER_NAME}"
#   rm -rf /tmp/clp

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${script_dir}/.test-common.sh"

CLUSTER_NAME="${CLUSTER_NAME:-clp-test-dedicated}"
NUM_COMPRESSION_NODES="${NUM_COMPRESSION_NODES:-2}"
NUM_QUERY_NODES="${NUM_QUERY_NODES:-2}"
COMPRESSION_WORKER_REPLICAS="${COMPRESSION_WORKER_REPLICAS:-2}"
QUERY_WORKER_REPLICAS="${QUERY_WORKER_REPLICAS:-2}"
REDUCER_REPLICAS="${REDUCER_REPLICAS:-2}"

echo "=== Multi-node test with dedicated worker nodes ==="
echo "Cluster: ${CLUSTER_NAME}"
echo "Compression nodes: ${NUM_COMPRESSION_NODES}"
echo "Query nodes: ${NUM_QUERY_NODES}"
echo "Compression workers: ${COMPRESSION_WORKER_REPLICAS}"
echo "Query workers: ${QUERY_WORKER_REPLICAS}"
echo "Reducers: ${REDUCER_REPLICAS}"
echo ""

echo "Deleting existing cluster if present..."
kind delete cluster --name "${CLUSTER_NAME}" 2>/dev/null || true

rm -rf "$CLP_HOME"
create_clp_directories
download_samples

total_workers=$((NUM_COMPRESSION_NODES + NUM_QUERY_NODES))

echo "Creating kind cluster..."
{
    cat <<EOF
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
- role: control-plane
  extraMounts:
  - hostPath: /home
    containerPath: /home
  - hostPath: $CLP_HOME
    containerPath: $CLP_HOME
  extraPortMappings:
  - containerPort: 30306
    hostPort: 30306
    protocol: TCP
  - containerPort: 30017
    hostPort: 30017
    protocol: TCP
  - containerPort: 30000
    hostPort: 30000
    protocol: TCP
  - containerPort: 30301
    hostPort: 30301
    protocol: TCP
EOF
    for ((i = 0; i < total_workers; i++)); do
        cat <<EOF
- role: worker
  extraMounts:
  - hostPath: /home
    containerPath: /home
  - hostPath: $CLP_HOME
    containerPath: $CLP_HOME
EOF
    done
} | kind create cluster --name "${CLUSTER_NAME}" --config=-

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
helm uninstall test --ignore-not-found 2>/dev/null || true
sleep 2
helm install test "${script_dir}" \
    --set "distributed=true" \
    --set "compressionWorker.replicas=${COMPRESSION_WORKER_REPLICAS}" \
    --set "compressionWorker.scheduling.nodeSelector.yscope\.io/nodeType=compression" \
    --set "queryWorker.replicas=${QUERY_WORKER_REPLICAS}" \
    --set "queryWorker.scheduling.nodeSelector.yscope\.io/nodeType=query" \
    --set "reducer.replicas=${REDUCER_REPLICAS}"

wait $SAMPLE_DOWNLOAD_PID
echo "Sample download and extraction complete"

wait_for_pods 300 5 5
