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
  - containerPort: 30080
    hostPort: 80
    protocol: TCP
  - containerPort: 30443
    hostPort: 443
    protocol: TCP
EOF

echo "Installing Gateway API CRDs..."
gateway_api_url="https://github.com/kubernetes-sigs/gateway-api/releases/download/v1.2.1"
kubectl apply -f "${gateway_api_url}/standard-install.yaml"

echo "Installing nginx-gateway-fabric..."
ngf_base_url="https://raw.githubusercontent.com/nginx/nginx-gateway-fabric/v1.6.2/deploy"
kubectl apply -f "${ngf_base_url}/crds.yaml"
kubectl apply -f "${ngf_base_url}/nodeport/deploy.yaml"
kubectl -n nginx-gateway patch service nginx-gateway --type=json --patch='[
  {"op": "replace", "path": "/spec/ports/0/nodePort", "value": 30080},
  {"op": "replace", "path": "/spec/ports/1/nodePort", "value": 30443}
]'
echo "Waiting for nginx-gateway-fabric controller pod to exist..."
kubectl -n nginx-gateway wait --for=create pod \
    --selector=app.kubernetes.io/name=nginx-gateway --timeout=120s
echo "Waiting for nginx-gateway-fabric controller to be ready..."
kubectl -n nginx-gateway wait --for=condition=Ready pod \
    --selector=app.kubernetes.io/name=nginx-gateway --timeout=120s

echo "Installing Helm chart with ClusterIP + Gateway API..."
helm uninstall test --ignore-not-found
sleep 2
# Word splitting is intentional: get_image_helm_args returns multiple --set flags.
# shellcheck disable=SC2046
helm install test "${script_dir}" \
    --set "allowHostAccessForSbinScripts=false" \
    --set "clpConfig.webui.serviceType=ClusterIP" \
    --set "clpConfig.api_server.serviceType=ClusterIP" \
    --set "gateway.enabled=true" \
    --set "gateway.className=nginx" \
    $(get_image_helm_args "${CLUSTER_NAME}" "clpPackage" "${CLP_PACKAGE_IMAGE}")

wait_for_cluster_ready

echo "Waiting for Gateway API resources to be ready..."
kubectl wait --for=condition=Programmed gateway/test-clp --timeout=120s
kubectl wait \
    --for=jsonpath='{.status.parents[0].conditions[?(@.type=="Accepted")].status}'=True \
    httproute/test-clp-webui --timeout=120s
kubectl wait \
    --for=jsonpath='{.status.parents[0].conditions[?(@.type=="Accepted")].status}'=True \
    httproute/test-clp-api-server --timeout=120s

echo "Validating Gateway routes..."
curl --fail --retry 30 --retry-all-errors --retry-delay 2 --silent --show-error \
    --output /dev/null "http://127.0.0.1/"
curl --fail --retry 30 --retry-all-errors --retry-delay 2 --silent --show-error \
    --output /dev/null "http://127.0.0.1/api/v2/health"
