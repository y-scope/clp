#!/usr/bin/env bash

# Common utilities for CLP Helm chart tests

set -o errexit
set -o nounset
set -o pipefail

CLP_HOME="${CLP_HOME:-/tmp/clp}"

# Waits for all jobs to complete and all non-job pods to be ready.
#
# @param {int} timeout_seconds Overall timeout in seconds
# @param {int} poll_interval_seconds Interval between status checks
# @param {int} wait_timeout_seconds Timeout for each kubectl wait call
# @param {string} kubectl_get_opts Additional options for kubectl get pods (optional)
# @return {int} 0 on success, 1 on timeout
wait_for_pods() {
    local timeout_seconds=$1
    local poll_interval_seconds=$2
    local wait_timeout_seconds=$3
    local kubectl_get_opts="${4:-}"

    echo "Waiting for all pods to be ready" \
        "(timeout=${timeout_seconds}s, poll=${poll_interval_seconds}s," \
        "wait=${wait_timeout_seconds}s)..."

    # Reset bash built-in SECONDS counter
    SECONDS=0

    while true; do
        sleep "${poll_interval_seconds}"
        # shellcheck disable=SC2086
        kubectl get pods ${kubectl_get_opts}

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

# Initializes the CLP home directory structure.
init_clp_home() {
    rm -rf "$CLP_HOME"
    mkdir -p "$CLP_HOME/var/"{data,log}/{database,queue,redis,results_cache} \
             "$CLP_HOME/var/data/"{archives,streams,staged-archives,staged-streams} \
             "$CLP_HOME/var/log/"{compression_scheduler,compression_worker,user} \
             "$CLP_HOME/var/log/"{query_scheduler,query_worker,reducer} \
             "$CLP_HOME/var/log/"{garbage_collector,api_server,log_ingestor,mcp_server} \
             "$CLP_HOME/var/tmp" \
             "$CLP_HOME/samples"
}

# Downloads sample datasets in the background.
# Sets SAMPLE_DOWNLOAD_PID to the background process ID.
download_samples() {
    wget -O - https://zenodo.org/records/10516402/files/postgresql.tar.gz?download=1 \
        | tar xz -C "$CLP_HOME/samples" &
    SAMPLE_DOWNLOAD_PID=$!
}

# Waits for sample download to complete.
wait_for_samples() {
    wait "$SAMPLE_DOWNLOAD_PID"
    echo "Sample download and extraction complete"
}

# Generates kind extra port mappings for control-plane node.
# These are the NodePort services exposed by the Helm chart.
generate_kind_port_mappings() {
    cat <<'EOF'
  extraPortMappings:
  - containerPort: 30306
    hostPort: 30306
    protocol: TCP
  - containerPort: 30017
    hostPort: 30017
    protocol: TCP
  - containerPort: 30000
    hostPort: 30000
    protocol: TCP
  - containerPort: 30301
    hostPort: 30301
    protocol: TCP
  - containerPort: 30302
    hostPort: 30302
    protocol: TCP
  - containerPort: 30800
    hostPort: 30800
    protocol: TCP
EOF
}
