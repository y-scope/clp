#!/usr/bin/env bash

# Multi-node cluster test with dedicated worker nodes for each worker type
# Demonstrates nodeSelector scheduling with separate node pools

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=.test-common.sh
source "${script_dir}/.test-common.sh"

CLUSTER_NAME="${CLUSTER_NAME:-clp-test-dedicated}"
NUM_COMPRESSION_NODES="${NUM_COMPRESSION_NODES:-2}"
NUM_QUERY_NODES="${NUM_QUERY_NODES:-2}"
COMPRESSION_WORKER_REPLICAS="${COMPRESSION_WORKER_REPLICAS:-2}"
QUERY_WORKER_REPLICAS="${QUERY_WORKER_REPLICAS:-2}"

echo "=== Multi-node CLP Helm Chart Test (Dedicated Workers) ==="
echo "Compression nodes: ${NUM_COMPRESSION_NODES}"
echo "Query nodes: ${NUM_QUERY_NODES}"
echo "Compression worker replicas: ${COMPRESSION_WORKER_REPLICAS}"
echo "Query worker replicas: ${QUERY_WORKER_REPLICAS}"
echo ""

kind delete cluster --name "${CLUSTER_NAME}" 2>/dev/null || true
init_clp_home
download_samples

total_workers=$((NUM_COMPRESSION_NODES + NUM_QUERY_NODES))
echo "Creating kind cluster with 1 control-plane + ${total_workers} worker nodes..."
{
    cat <<EOF
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
- role: control-plane
  extraMounts:
  - hostPath: /home
    containerPath: /home
  - hostPath: ${CLP_HOME}
    containerPath: ${CLP_HOME}
$(generate_kind_port_mappings)
EOF
    for _ in $(seq 1 "${total_workers}"); do
        cat <<EOF
- role: worker
  extraMounts:
  - hostPath: /home
    containerPath: /home
  - hostPath: ${CLP_HOME}
    containerPath: ${CLP_HOME}
EOF
    done
} | kind create cluster --name "${CLUSTER_NAME}" --config=-

# Label worker nodes for dedicated workloads
echo "Labeling worker nodes..."
worker_nodes=($(kubectl get nodes -l '!node-role.kubernetes.io/control-plane' -o name))

for i in $(seq 0 $((NUM_COMPRESSION_NODES - 1))); do
    kubectl label "${worker_nodes[$i]}" yscope.io/nodeType=compression --overwrite
done

for i in $(seq "${NUM_COMPRESSION_NODES}" $((total_workers - 1))); do
    kubectl label "${worker_nodes[$i]}" yscope.io/nodeType=query --overwrite
done

echo ""
echo "Cluster nodes:"
kubectl get nodes -L yscope.io/nodeType
echo ""

# Remove control-plane taint to allow scheduling on control-plane if needed
kubectl taint nodes --selector=node-role.kubernetes.io/control-plane \
    node-role.kubernetes.io/control-plane:NoSchedule- 2>/dev/null || true

helm uninstall test --ignore-not-found 2>/dev/null || true
sleep 2

echo "Installing Helm chart with dedicated worker nodes..."
helm install test . \
    --set "distributed=true" \
    --set "compressionWorker.replicas=${COMPRESSION_WORKER_REPLICAS}" \
    --set "compressionWorker.scheduling.nodeSelector.yscope\.io/nodeType=compression" \
    --set "queryWorker.replicas=${QUERY_WORKER_REPLICAS}" \
    --set "queryWorker.scheduling.nodeSelector.yscope\.io/nodeType=query"

wait_for_samples
wait_for_pods 300 5 5

echo ""
echo "=== Pod Distribution ==="
kubectl get pods -o wide

echo ""
echo "To clean up:"
echo "  kind delete cluster --name ${CLUSTER_NAME}"
