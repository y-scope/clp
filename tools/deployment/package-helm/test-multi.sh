#!/usr/bin/env bash

# Multi-node cluster test for CLP Helm chart
# Tests distributed storage mode with multiple worker replicas

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=.test-common.sh
source "${script_dir}/.test-common.sh"

CLUSTER_NAME="${CLUSTER_NAME:-clp-test-multi}"
NUM_WORKER_NODES="${NUM_WORKER_NODES:-2}"
COMPRESSION_WORKER_REPLICAS="${COMPRESSION_WORKER_REPLICAS:-2}"
QUERY_WORKER_REPLICAS="${QUERY_WORKER_REPLICAS:-2}"

echo "=== Multi-node CLP Helm Chart Test ==="
echo "Worker nodes: ${NUM_WORKER_NODES}"
echo "Compression worker replicas: ${COMPRESSION_WORKER_REPLICAS}"
echo "Query worker replicas: ${QUERY_WORKER_REPLICAS}"
echo ""

kind delete cluster --name "${CLUSTER_NAME}" 2>/dev/null || true
init_clp_home
download_samples

echo "Creating kind cluster with 1 control-plane + ${NUM_WORKER_NODES} worker nodes..."
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
    for _ in $(seq 1 "${NUM_WORKER_NODES}"); do
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

# Label worker nodes for identification
echo "Labeling worker nodes..."
for node in $(kubectl get nodes -l '!node-role.kubernetes.io/control-plane' -o name); do
    kubectl label "${node}" node-role.kubernetes.io/worker="" --overwrite
done

echo ""
echo "Cluster nodes:"
kubectl get nodes --show-labels
echo ""

# Remove control-plane taint to allow scheduling on control-plane if needed
kubectl taint nodes --selector=node-role.kubernetes.io/control-plane \
    node-role.kubernetes.io/control-plane:NoSchedule- 2>/dev/null || true

helm uninstall test --ignore-not-found 2>/dev/null || true
sleep 2

echo "Installing Helm chart with distributed storage mode..."
helm install test . \
    --set "distributed=true" \
    --set "compressionWorker.replicas=${COMPRESSION_WORKER_REPLICAS}" \
    --set "queryWorker.replicas=${QUERY_WORKER_REPLICAS}"

wait_for_samples
wait_for_pods 300 5 5

echo ""
echo "=== Pod Distribution ==="
kubectl get pods -o wide

echo ""
echo "To clean up:"
echo "  kind delete cluster --name ${CLUSTER_NAME}"
