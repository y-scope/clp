#!/bin/bash

# TODO: to be deleted / migrated to integration tests

kind delete cluster --name clp-test
sudo rm -rf /tmp/clp
mkdir -p /tmp/clp/var/{data,log}/{database,queue,redis,results-cache,compression-scheduler,compression-worker,query-scheduler,query-worker}
mkdir -p /tmp/clp/var/data/{archives,staged-streams,streams}
mkdir -p /tmp/clp/var/tmp
mkdir -p /tmp/clp/samples

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
    - containerPort: 30400
      hostPort: 30400
      protocol: TCP
EOF

cp /home/junhao/samples/postgresql.jsonl /tmp/clp/postgresql.jsonl

helm uninstall test || true
sleep 2
helm install test . --set image.clpPackage.tag=nightly
ls -l /tmp/clp/var/data/
