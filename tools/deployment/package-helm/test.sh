#!/usr/bin/env bash

# Single-node cluster test for CLP Helm chart
# TODO: Migrate into integration test

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=.test-common.sh
source "${script_dir}/.test-common.sh"

CLUSTER_NAME="${CLUSTER_NAME:-clp-test}"

echo "=== Single-node CLP Helm Chart Test ==="
echo ""

kind delete cluster --name "${CLUSTER_NAME}" 2>/dev/null || true
init_clp_home
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
  - hostPath: ${CLP_HOME}
    containerPath: ${CLP_HOME}
$(generate_kind_port_mappings)
EOF
} | kind create cluster --name "${CLUSTER_NAME}" --config=-

helm uninstall test --ignore-not-found 2>/dev/null || true
sleep 2

echo "Installing Helm chart..."
helm install test .

wait_for_samples
wait_for_pods 300 5 5

echo ""
echo "To clean up:"
echo "  kind delete cluster --name ${CLUSTER_NAME}"
