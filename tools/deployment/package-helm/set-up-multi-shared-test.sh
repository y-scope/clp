#!/usr/bin/env bash

# Multi-node cluster test with shared worker nodes
# Both compression and query workers share the same node pool
#
# To clean up after running:
#   kind delete cluster --name "${CLUSTER_NAME}"
#   rm -rf /tmp/clp

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${script_dir}/.test-common.sh"

CLUSTER_NAME="${CLUSTER_NAME:-clp-test-multi}"
NUM_WORKER_NODES="${NUM_WORKER_NODES:-2}"
COMPRESSION_WORKER_REPLICAS="${COMPRESSION_WORKER_REPLICAS:-2}"
QUERY_WORKER_REPLICAS="${QUERY_WORKER_REPLICAS:-2}"
REDUCER_REPLICAS="${REDUCER_REPLICAS:-2}"

echo "=== Multi-node test with shared worker nodes ==="
echo "Cluster: ${CLUSTER_NAME}"
echo "Worker nodes: ${NUM_WORKER_NODES}"
echo "Compression workers: ${COMPRESSION_WORKER_REPLICAS}"
echo "Query workers: ${QUERY_WORKER_REPLICAS}"
echo "Reducers: ${REDUCER_REPLICAS}"
echo ""

echo "Deleting existing cluster if present..."
kind delete cluster --name "${CLUSTER_NAME}" 2>/dev/null || true

rm -rf "$CLP_HOME"
create_clp_directories
download_samples

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
    for ((i = 0; i < NUM_WORKER_NODES; i++)); do
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

echo "Installing Helm chart..."
helm uninstall test --ignore-not-found 2>/dev/null || true
sleep 2
helm install test "${script_dir}" \
    --set "distributed=true" \
    --set "compressionWorker.replicas=${COMPRESSION_WORKER_REPLICAS}" \
    --set "queryWorker.replicas=${QUERY_WORKER_REPLICAS}" \
    --set "reducer.replicas=${REDUCER_REPLICAS}"

wait $SAMPLE_DOWNLOAD_PID
echo "Sample download and extraction complete"

wait_for_pods 300 5 5
