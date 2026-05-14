#!/usr/bin/env bash

# Single-node cluster setup with ClusterIP services + Gateway API (nginx-gateway-fabric)
# Validates that webui and api-server are reachable through a Gateway controller
# TODO: Migrate into integration test

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

CLP_HOME="${CLP_HOME:-/tmp/clp}"
CLUSTER_NAME="${CLUSTER_NAME:-clp-test}"

# shellcheck source=.set-up-common.sh
source "${script_dir}/.set-up-common.sh"

parse_common_args "$@"

echo "=== Single-node Gateway API setup ==="
echo "Cluster: ${CLUSTER_NAME}"
echo ""

prepare_environment "${CLUSTER_NAME}"

echo "Creating kind cluster with Gateway port mappings..."
cat <<EOF | kind create cluster --name "${CLUSTER_NAME}" --config=-
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
  - containerPort: 80
    hostPort: 80
    protocol: TCP
  - containerPort: 443
    hostPort: 443
    protocol: TCP
EOF

echo "Installing Gateway API CRDs..."
kubectl apply -f https://github.com/kubernetes-sigs/gateway-api/releases/download/v1.2.1/standard-install.yaml

echo "Installing nginx-gateway-fabric..."
kubectl apply -f https://github.com/nginx/nginx-gateway-fabric/releases/download/v1.6.2/nginx-gateway-fabric.yaml
echo "Waiting for nginx-gateway-fabric controller pod to exist..."
until kubectl -n nginx-gateway get pods \
    --selector=app.kubernetes.io/name=nginx-gateway-fabric 2>/dev/null | grep -q "nginx-gateway"; do
    sleep 2
done
echo "Waiting for nginx-gateway-fabric controller to be ready..."
kubectl -n nginx-gateway wait --for=condition=Ready pod \
    --selector=app.kubernetes.io/name=nginx-gateway-fabric --timeout=120s

echo "Installing Helm chart with ClusterIP + Gateway API..."
helm uninstall test --ignore-not-found
sleep 2
# Word splitting is intentional: get_image_helm_args returns multiple --set flags.
# shellcheck disable=SC2046
helm install test "${script_dir}" \
    --set "clpConfig.webui.serviceType=ClusterIP" \
    --set "clpConfig.api_server.serviceType=ClusterIP" \
    --set "gateway.enabled=true" \
    --set "gateway.className=nginx" \
    $(get_image_helm_args "${CLUSTER_NAME}" "${CLP_PACKAGE_IMAGE}")

wait_for_cluster_ready
