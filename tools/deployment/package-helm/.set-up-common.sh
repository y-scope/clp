#!/usr/bin/env bash

# Common utilities for Helm chart testing
# Source this file from test scripts

set -o errexit
set -o nounset
set -o pipefail

CLP_HOME="${CLP_HOME:-/tmp/clp}"

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

# Creates required directories for CLP data and logs
create_clp_directories() {
    echo "Creating CLP directories at ${CLP_HOME}..."
    mkdir -p  "$CLP_HOME/var/"{data,log}/{database,queue,redis,results_cache} \
              "$CLP_HOME/var/data/"{archives,streams,staged-archives,staged-streams} \
              "$CLP_HOME/var/log/"{compression_scheduler,compression_worker,user} \
              "$CLP_HOME/var/log/"{query_scheduler,query_worker,reducer} \
              "$CLP_HOME/var/log/{api_server,garbage_collector}" \
              "$CLP_HOME/var/tmp" \
              "$CLP_HOME/samples"
}

# Downloads sample datasets in the background
# Sets SAMPLE_DOWNLOAD_PID global variable
download_samples() {
    echo "Downloading sample datasets..."
    wget -O - https://zenodo.org/records/10516402/files/postgresql.tar.gz?download=1 \
      | tar xz -C "$CLP_HOME/samples" &
    SAMPLE_DOWNLOAD_PID=$!

    # Generate sample log file for garbage collector testing
    cat <<EOF > "$CLP_HOME/samples/test-gc.jsonl"
{"timestamp": $(date +%s%3N), "level": "INFO", "message": "User login successful"}
{"timestamp": $(date +%s%3N), "level": "ERROR", "message": "Database connection failed"}
EOF
}
