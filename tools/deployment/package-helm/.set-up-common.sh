#!/usr/bin/env bash

# Common utilities for Helm chart set-up scripts
# Source this file from set-up-*.sh scripts

set -o errexit
set -o nounset
set -o pipefail

# Creates required directories for CLP data
create_clp_directories() {
    echo "Creating CLP directories at ${CLP_HOME}..."
    mkdir -p "$CLP_HOME/samples"
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

# Cleans up existing cluster and prepares environment
# @param {string} cluster_name Name of the kind cluster
prepare_environment() {
    local cluster_name=$1

    echo "Deleting existing cluster if present..."
    kind delete cluster --name "${cluster_name}" 2>/dev/null || true

    rm -rf "$CLP_HOME"
    create_clp_directories
    download_samples
}

# Loads a local Docker image into the kind cluster and returns the helm --set
# flags for using it. If image is not specified, returns empty string.
#
# @param {string} cluster_name Name of the kind cluster
# @param {string} [image] Docker image (e.g., "clp-package:dev-junhao-a6bf")
# @return Prints helm --set flags to stdout
get_image_helm_args() {
    local cluster_name=$1
    local image="${2:-}"

    if [[ -z "${image}" ]]; then
        return
    fi

    echo "Loading local image '${image}' into kind cluster..." >&2
    kind load docker-image "${image}" --name "${cluster_name}" >&2

    # Split "repo:tag" on the last colon whose right-hand side contains no '/'
    # (so registry ports like localhost:5000/repo are not mistaken for tags).
    if [[ "${image}" =~ ^(.+):([^:/]+)$ ]]; then
        local repo="${BASH_REMATCH[1]}"
        local tag="${BASH_REMATCH[2]}"
    else
        echo "Error: '${image}' is not a valid image reference (expected repo:tag)." >&2
        return 1
    fi
    echo "--set" "image.clpPackage.repository=${repo}" \
         "--set" "image.clpPackage.tag=${tag}" \
         "--set" "image.clpPackage.pullPolicy=Never"
}

# Parses common arguments shared across set-up scripts.
# Sets CLP_PACKAGE_IMAGE global variable.
#
# @param {string[]} args Script arguments
parse_common_args() {
    CLP_PACKAGE_IMAGE=""
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --clp-package-image)
                if [[ $# -lt 2 || "$2" == --* ]]; then
                    echo "Error: '--clp-package-image' requires a value." >&2
                    exit 1
                fi
                CLP_PACKAGE_IMAGE="$2"
                shift 2
                ;;
            *)
                echo "Unknown argument: $1" >&2
                exit 1
                ;;
        esac
    done
}

# Generates kind cluster configuration YAML
#
# @param {int} num_workers Number of worker nodes (0 for single-node cluster)
generate_kind_config() {
    local num_workers=${1:-0}

    cat <<EOF
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
  - containerPort: 30000
    hostPort: 30000
    protocol: TCP
  - containerPort: 30017
    hostPort: 30017
    protocol: TCP
  - containerPort: 30301
    hostPort: 30301
    protocol: TCP
  - containerPort: 30302
    hostPort: 30302
    protocol: TCP
  - containerPort: 30306
    hostPort: 30306
    protocol: TCP
  - containerPort: 30800
    hostPort: 30800
    protocol: TCP
EOF

    for ((i = 0; i < num_workers; i++)); do
        cat <<EOF
- role: worker
  extraMounts:
  - hostPath: /home
    containerPath: /home
  - hostPath: $CLP_HOME
    containerPath: $CLP_HOME
EOF
    done
}

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

# Waits for sample download to complete and all pods to be ready
wait_for_cluster_ready() {
    if wait "$SAMPLE_DOWNLOAD_PID"; then
        echo "Sample download and extraction complete"
    else
        echo "ERROR: Sample download failed"
        return 1
    fi

    wait_for_pods 300 5 5
}
