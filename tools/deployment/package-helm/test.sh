#!/usr/bin/env bash

# TODO: Migrate into integration test

set -o errexit
set -o nounset
set -o pipefail

kind delete cluster --name clp-test
rm -rf /tmp/clp
mkdir -p /tmp/clp/var/{data,log}/{database,queue,redis,results-cache,compression-scheduler,compression-worker,query-scheduler,query-worker,reducer,garbage-collector,api-server,mcp-server}
mkdir -p /tmp/clp/var/data/{archives,staged-archives,staged-streams,streams}
mkdir -p /tmp/clp/var/log/user
mkdir -p /tmp/clp/var/tmp
mkdir -p /tmp/clp/samples

# Download sample datasets in the background
wget -O - https://zenodo.org/records/10516402/files/postgresql.tar.gz?download=1 \
  | tar xz -C /tmp/clp/samples &
SAMPLE_DOWNLOAD_PID=$!

cat <<EOF | kind create cluster --name clp-test --config=-
  kind: Cluster
  apiVersion: kind.x-k8s.io/v1alpha4
  nodes:
  - role: control-plane
    extraMounts:
    - hostPath: /home
      containerPath: /home
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
    - containerPort: 30301
      hostPort: 30301
      protocol: TCP
    - containerPort: 30800
      hostPort: 30800
      protocol: TCP
EOF

helm uninstall test || true
sleep 2
helm install test .
ls -l /tmp/clp/var/data/

wait $SAMPLE_DOWNLOAD_PID
echo "Sample download and extraction complete"