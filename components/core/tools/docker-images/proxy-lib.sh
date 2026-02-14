#!/usr/bin/env bash

# Shared library for corporate proxy support in Docker image builds.
# Source this file from build.sh scripts.
#
# Problem: In corporate environments, TLS-intercepting proxies (e.g., Zscaler,
# Fortinet, Palo Alto, Symantec/Blue Coat) replace upstream SSL certificates
# with ones signed by the organization's own CA. Tools inside Docker containers
# (curl, dnf, pip, etc.) reject these certificates because the corporate CA
# isn't in the container's trust store.
#
# Solution: This library detects the host's CA bundle, copies it into the Docker
# build context, and forwards proxy environment variables as build args. A
# companion script (setup-corporate-proxy.sh) runs inside the container to
# install the CA bundle into the system trust store.
#
# When no CA bundle is found on the host, everything is a no-op — builds work
# identically to before.

# Detects the host's CA certificate bundle.
# Returns the path via stdout, or returns 1 if not found.
detect_ca_bundle() {
    local ca_paths=(
        "${SSL_CERT_FILE:-}"                # User/corporate override
        /etc/ssl/certs/ca-certificates.crt  # Debian/Ubuntu/Alpine
        /etc/pki/tls/certs/ca-bundle.crt    # RHEL/CentOS/Fedora
        /etc/ssl/cert.pem                   # macOS
        /etc/ssl/ca-bundle.pem              # openSUSE
    )

    for path in "${ca_paths[@]}"; do
        if [[ -n "$path" && -f "$path" ]]; then
            echo "$path"
            return 0
        fi
    done

    return 1
}

# Copies the host's CA bundle into the build context so the Dockerfile can
# install it into the container's trust store. On a normal workstation this
# copies the standard public CA bundle (redundant but harmless). On a corporate
# machine, the bundle includes the organization's CA — which the container needs.
# Creates an empty file if no bundle is found (so COPY always succeeds and the
# in-container setup script is a no-op).
# Arguments:
#   $1 - component_root (path to components/core/)
prepare_ca_cert_for_build() {
    local component_root="$1"
    local dest="${component_root}/tools/scripts/lib_install/ca-certificates.crt"

    local ca_bundle
    if ca_bundle="$(detect_ca_bundle)"; then
        echo "Corporate proxy support: copying CA bundle from ${ca_bundle}"
        cp "$ca_bundle" "$dest"
    else
        echo "Corporate proxy support: no CA bundle found on host (no-op)"
        touch "$dest"
    fi
}

# Removes the temporary CA certificate file from the build context.
# Arguments:
#   $1 - component_root (path to components/core/)
cleanup_ca_cert() {
    local component_root="$1"
    local cert_file="${component_root}/tools/scripts/lib_install/ca-certificates.crt"

    if [[ -f "$cert_file" ]]; then
        rm "$cert_file"
    fi
}

# Finalizes the build command array and executes the Docker build.
# Adds proxy build args, an optional mirror override, DOCKER_PULL support,
# git metadata labels, then runs the build.
# Arguments:
#   $1 - name of the bash array variable holding the build command
#   $2 - script_dir (used for git label metadata)
#   $3 - (optional) name of the mirror override env var (e.g., DNF_MIRROR_BASE_URL)
finalize_build() {
    local -n _build_cmd_ref=$1
    local script_dir="$2"
    local mirror_var="${3:-}"

    add_proxy_build_args "$1"

    if [[ -n "$mirror_var" ]] && [[ -n "${!mirror_var:-}" ]]; then
        _build_cmd_ref+=(--build-arg "${mirror_var}=${!mirror_var}")
    fi

    if [[ "${DOCKER_PULL:-false}" == "true" ]]; then
        _build_cmd_ref+=(--pull)
    fi

    if command -v git >/dev/null \
        && git -C "$script_dir" rev-parse --is-inside-work-tree >/dev/null 2>&1
    then
        _build_cmd_ref+=(
            --label "org.opencontainers.image.revision=$(git -C "$script_dir" rev-parse HEAD)"
            --label "org.opencontainers.image.source=$(git -C "$script_dir" remote get-url origin)"
        )
    fi

    echo "Running: ${_build_cmd_ref[*]}"
    "${_build_cmd_ref[@]}"
}

# Appends --build-arg flags for proxy environment variables to the given
# build command array. Also handles Docker network mode:
#   - If DOCKER_NETWORK is set, uses that value (e.g., DOCKER_NETWORK=host)
#   - Otherwise, if any proxy URL points to localhost/127.0.0.1, defaults to
#     --network host so the build container can reach the host's proxy
#   - Otherwise, uses Docker's default (bridge) network
# Arguments:
#   $1 - name of the bash array variable holding the build command
add_proxy_build_args() {
    local -n _build_cmd=$1

    local proxy_vars=(
        HTTP_PROXY http_proxy
        HTTPS_PROXY https_proxy
        ALL_PROXY all_proxy
    )
    local no_proxy_vars=(
        NO_PROXY no_proxy
    )

    local has_localhost_proxy=false
    for var in "${proxy_vars[@]}"; do
        if [[ -n "${!var:-}" ]]; then
            _build_cmd+=(--build-arg "${var}=${!var}")
            if [[ "${!var}" =~ (localhost|127\.0\.0\.1) ]]; then
                has_localhost_proxy=true
            fi
        fi
    done
    for var in "${no_proxy_vars[@]}"; do
        if [[ -n "${!var:-}" ]]; then
            _build_cmd+=(--build-arg "${var}=${!var}")
        fi
    done

    if [[ -n "${DOCKER_NETWORK:-}" ]]; then
        _build_cmd+=(--network "${DOCKER_NETWORK}")
    elif [[ "$has_localhost_proxy" == "true" ]]; then
        _build_cmd+=(--network host)
    fi
}
