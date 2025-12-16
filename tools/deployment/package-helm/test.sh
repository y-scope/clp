#!/usr/bin/env bash

# TODO: Migrate into integration test

set -o errexit
set -o nounset
set -o pipefail

# Waits for all jobs to complete and all non-job pods to be ready.
#
# @param {int} timeout_seconds Overall timeout in seconds
# @param {int} poll_interval_seconds Interval between status checks
# @param {int} wait_timeout_seconds Timeout for each kubectl wait call
# @return {int} 0 on success, 1 on timeout
wait_for_pods() {
    local timeout_seconds=$1
    local poll_interval_seconds=$2
    local wait_timeout_seconds=$3

    echo "Waiting for all pods to be ready" \
        "(timeout=${timeout_seconds}s, poll=${poll_interval_seconds}s," \
        "wait=${wait_timeout_seconds}s)..."

    # Reset bash built-in SECONDS counter
    SECONDS=0

    while true; do
        sleep "${poll_interval_seconds}"
        kubectl get pods

        if kubectl wait job \
                --all \
                --for=condition=Complete \
                --timeout="${wait_timeout_seconds}s" 2>/dev/null \
            && kubectl wait pods \
                --all \
                --selector='!job-name' \
                --for=condition=Ready \
                --timeout="${wait_timeout_seconds}s" 2>/dev/null
        then
            echo "All jobs completed and services are ready."
            return 0
        fi

        if [[ ${SECONDS} -ge ${timeout_seconds} ]]; then
            echo "ERROR: Timed out waiting for pods to be ready"
            return 1
        fi

        echo "---"
    done
}

kind delete cluster --name clp-test
rm -rf /tmp/clp
mkdir -p /tmp/clp/var/{data,log}/{database,queue,redis,results_cache}

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

helm uninstall test --ignore-not-found
sleep 2
helm install test .

wait_for_pods 300 5 5
