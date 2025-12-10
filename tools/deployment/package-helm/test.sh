#!/usr/bin/env bash

# TODO: Migrate into integration test

set -o errexit
set -o nounset
set -o pipefail

kind delete cluster --name clp-test
rm -rf /tmp/clp
mkdir -p /tmp/clp/var/{data,log}/{database,queue,redis,results-cache}

cat <<EOF | kind create cluster --name clp-test --config=-
  kind: Cluster
  apiVersion: kind.x-k8s.io/v1alpha4
  nodes:
  - role: control-plane
    extraMounts:
    - hostPath: /tmp/clp
      containerPath: /tmp/clp
    extraPortMappings:
    - containerPort: 30306
      hostPort: 30306
      protocol: TCP
    - containerPort: 30017
      hostPort: 30017
      protocol: TCP
EOF

helm uninstall test || true
sleep 2
helm install test .
