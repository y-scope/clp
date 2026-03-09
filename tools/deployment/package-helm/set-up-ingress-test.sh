#!/usr/bin/env bash

# Single-node cluster setup with ClusterIP services + nginx Ingress
# Validates that webui and api-server are reachable through an Ingress controller
# TODO: Migrate into integration test

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

CLP_HOME="${CLP_HOME:-/tmp/clp}"
CLUSTER_NAME="${CLUSTER_NAME:-clp-test}"

# shellcheck source=.set-up-common.sh
source "${script_dir}/.set-up-common.sh"

parse_common_args "$@"

echo "=== Single-node Ingress setup ==="
echo "Cluster: ${CLUSTER_NAME}"
echo ""

prepare_environment "${CLUSTER_NAME}"

echo "Creating kind cluster with Ingress port mappings..."
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
  kubeadmConfigPatches:
  - |
    kind: InitConfiguration
    nodeRegistration:
      kubeletExtraArgs:
        node-labels: "ingress-ready=true"
  extraPortMappings:
  - containerPort: 80
    hostPort: 80
    protocol: TCP
  - containerPort: 443
    hostPort: 443
    protocol: TCP
EOF

echo "Installing nginx-ingress controller..."
kubectl apply -f https://kind.sigs.k8s.io/examples/ingress/deploy-ingress-nginx.yaml
echo "Waiting for nginx-ingress controller pod to exist..."
until kubectl -n ingress-nginx get pods \
    --selector=app.kubernetes.io/component=controller 2>/dev/null | grep -q "controller"; do
    sleep 2
done
echo "Waiting for nginx-ingress controller to be ready..."
kubectl -n ingress-nginx wait --for=condition=Ready pod \
    --selector=app.kubernetes.io/component=controller --timeout=120s

echo "Installing Helm chart with ClusterIP + Ingress..."
helm uninstall test --ignore-not-found
sleep 2
# Word splitting is intentional: get_image_helm_args returns multiple --set flags.
# shellcheck disable=SC2046
helm install test "${script_dir}" \
    --set "clpConfig.webui.serviceType=ClusterIP" \
    --set "clpConfig.api_server.serviceType=ClusterIP" \
    --set "ingress.enabled=true" \
    --set "ingress.className=nginx" \
    $(get_image_helm_args "${CLUSTER_NAME}" "${CLP_PACKAGE_IMAGE}")

wait_for_cluster_ready
