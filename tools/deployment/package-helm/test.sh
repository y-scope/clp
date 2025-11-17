#!/bin/bash

# TODO: to be deleted / migrated to integration tests

kind delete cluster --name clp-test
rm -rf /tmp/clp
mkdir -p /tmp/clp/var/{data,log}/database

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
EOF

helm uninstall test || true
sleep 2
helm install test .
kubectl get pods -w