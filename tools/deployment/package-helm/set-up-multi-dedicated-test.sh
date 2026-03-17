#!/usr/bin/env bash

# Multi-node cluster setup with dedicated worker nodes for each worker type
# Demonstrates nodeSelector scheduling with separate node pools
# TODO: Migrate into integration test

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

CLP_HOME="${CLP_HOME:-/tmp/clp}"
CLUSTER_NAME="${CLUSTER_NAME:-clp-test}"
NUM_COMPRESSION_NODES="${NUM_COMPRESSION_NODES:-2}"
NUM_QUERY_NODES="${NUM_QUERY_NODES:-2}"
NUM_PRESTO_NODES="${NUM_PRESTO_NODES:-0}"
COMPRESSION_WORKER_REPLICAS="${COMPRESSION_WORKER_REPLICAS:-2}"
QUERY_WORKER_REPLICAS="${QUERY_WORKER_REPLICAS:-2}"
REDUCER_REPLICAS="${REDUCER_REPLICAS:-2}"
PRESTO_WORKER_REPLICAS="${PRESTO_WORKER_REPLICAS:-0}"

# shellcheck source=.set-up-common.sh
source "${script_dir}/.set-up-common.sh"

parse_common_args "$@"

# When --presto is passed, use non-zero defaults for Presto nodes/replicas if not explicitly set.
if [[ "${ENABLE_PRESTO}" == "true" ]]; then
    NUM_PRESTO_NODES="${NUM_PRESTO_NODES:-2}"
    : "${NUM_PRESTO_NODES:=2}"
    [[ "${NUM_PRESTO_NODES}" -eq 0 ]] && NUM_PRESTO_NODES=2
    PRESTO_WORKER_REPLICAS="${PRESTO_WORKER_REPLICAS:-2}"
    [[ "${PRESTO_WORKER_REPLICAS}" -eq 0 ]] && PRESTO_WORKER_REPLICAS=2
fi

echo "=== Multi-node setup with dedicated worker nodes ==="
echo "Cluster: ${CLUSTER_NAME}"
echo "Compression nodes: ${NUM_COMPRESSION_NODES}"
echo "Query nodes: ${NUM_QUERY_NODES}"
echo "Presto nodes: ${NUM_PRESTO_NODES}"
echo "Compression workers: ${COMPRESSION_WORKER_REPLICAS}"
echo "Query workers: ${QUERY_WORKER_REPLICAS}"
echo "Reducers: ${REDUCER_REPLICAS}"
echo "Presto workers: ${PRESTO_WORKER_REPLICAS}"
echo "Presto: ${ENABLE_PRESTO}"
echo ""

prepare_environment "${CLUSTER_NAME}"

total_workers=$((NUM_COMPRESSION_NODES + NUM_QUERY_NODES + NUM_PRESTO_NODES))

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
query_end=$((NUM_COMPRESSION_NODES + NUM_QUERY_NODES))
for ((i = NUM_COMPRESSION_NODES; i < query_end; i++)); do
    echo "Labeling ${worker_nodes[$i]} as query node"
    kubectl label node "${worker_nodes[$i]}" yscope.io/nodeType=query --overwrite
done

# Label Presto nodes
for ((i = query_end; i < total_workers; i++)); do
    echo "Labeling ${worker_nodes[$i]} as presto node"
    kubectl label node "${worker_nodes[$i]}" yscope.io/nodeType=presto --overwrite
done

# Pre-create shared-data PVs with hostPath (no node affinity) so PVCs bind to them instead of
# dynamically provisioned node-local volumes. Without this, the local-path-provisioner pins PVs to
# whichever node claims them first, which conflicts with nodeSelector when workers are on dedicated
# node pools.
echo "Creating shared-data PersistentVolumes..."
kubectl apply -f - <<EOF
apiVersion: v1
kind: PersistentVolume
metadata:
  name: test-clp-shared-data-archives
spec:
  capacity:
    storage: 50Gi
  accessModes: [ReadWriteOnce]
  storageClassName: ""
  claimRef:
    namespace: default
    name: test-clp-shared-data-archives
  hostPath:
    path: /var/data/archives
    type: DirectoryOrCreate
---
apiVersion: v1
kind: PersistentVolume
metadata:
  name: test-clp-shared-data-streams
spec:
  capacity:
    storage: 20Gi
  accessModes: [ReadWriteOnce]
  storageClassName: ""
  claimRef:
    namespace: default
    name: test-clp-shared-data-streams
  hostPath:
    path: /var/data/streams
    type: DirectoryOrCreate
EOF

echo "Installing Helm chart..."
helm uninstall test --ignore-not-found
sleep 2
# Word splitting is intentional: helper functions return multiple --set flags.
# shellcheck disable=SC2046
helm install test "${script_dir}" \
    --set "distributedDeployment=true" \
    --set "compressionWorker.replicas=${COMPRESSION_WORKER_REPLICAS}" \
    --set "compressionWorker.scheduling.nodeSelector.yscope\.io/nodeType=compression" \
    --set "queryWorker.replicas=${QUERY_WORKER_REPLICAS}" \
    --set "queryWorker.scheduling.nodeSelector.yscope\.io/nodeType=query" \
    --set "reducer.replicas=${REDUCER_REPLICAS}" \
    --set "reducer.scheduling.nodeSelector.yscope\.io/nodeType=query" \
    --set "prestoWorker.replicas=${PRESTO_WORKER_REPLICAS}" \
    --set "prestoWorker.scheduling.nodeSelector.yscope\.io/nodeType=presto" \
    $(get_presto_helm_args) \
    $(get_image_helm_args "${CLUSTER_NAME}" "${CLP_PACKAGE_IMAGE}")

wait_for_cluster_ready
