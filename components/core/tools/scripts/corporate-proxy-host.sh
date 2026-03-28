#!/usr/bin/env bash

# HOST-SIDE half of the two-part corporate proxy support system for Docker image builds.
# Counterpart: tools/scripts/corporate-proxy-container.sh (runs inside the container).
#
# Two-part flow:
#   1. (Host)      This library is sourced by each docker-images/*/build.sh. It detects the host's
#                  CA bundle, stages it into the build context, and injects proxy env vars as
#                  Docker --build-arg flags before invoking `docker build`.
#   2. (Container) corporate-proxy-container.sh runs as a Dockerfile RUN step. It installs the
#                  staged CA bundle into the container's system trust store so that tools like curl,
#                  dnf, and pip can reach the internet through a TLS-intercepting proxy.
#
# Problem: In corporate environments, TLS-intercepting proxies (e.g., Zscaler, Fortinet, Palo Alto,
# Symantec/Blue Coat) replace upstream SSL certificates with ones signed by the organization's own
# CA. Tools inside Docker containers reject these certificates because the corporate CA isn't in the
# container's trust store.
#
# If no CA bundle is found on the host, the build fails with an error — every supported
# platform (Linux, macOS) should have a system CA bundle.

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

# Copies the host's CA bundle into the build context so the Dockerfile can install it into the
# container's trust store. On a normal workstation this copies the standard public CA bundle
# (redundant but harmless). On a corporate machine, the bundle includes the organization's CA —
# which the container needs. Errors if no bundle is found on the host.
#
# NOTE: ca-certificates.crt is gitignored and generated at build time. CI workflows
# must stage the runner's system CA bundle before docker build. See the
# clp-core-build-containers action for the CI equivalent of this function.
# Arguments:
#   $1 - component_root (path to components/core/)
prepare_ca_cert_for_build() {
    local component_root="$1"
    local dest="${component_root}/tools/scripts/ca-certificates.crt"

    local ca_bundle
    if ca_bundle="$(detect_ca_bundle)"; then
        echo "Corporate proxy support: copying CA bundle from ${ca_bundle}"
        cp "$ca_bundle" "$dest"
    else
        echo >&2 "ERROR: No CA certificate bundle found on host."
        echo >&2 "  Expected one of:"
        echo >&2 "    /etc/ssl/certs/ca-certificates.crt (Debian/Ubuntu/Alpine)"
        echo >&2 "    /etc/pki/tls/certs/ca-bundle.crt   (RHEL/CentOS)"
        echo >&2 "    /etc/ssl/cert.pem                   (macOS)"
        exit 1
    fi
}

# Removes the generated CA certificate file from the build context.
# Arguments:
#   $1 - component_root (path to components/core/)
cleanup_ca_cert() {
    local component_root="$1"
    local cert_file="${component_root}/tools/scripts/ca-certificates.crt"

    if [[ -f "$cert_file" ]]; then
        rm "$cert_file"
    fi
}

# Finalizes the build command array and executes the Docker build. Adds proxy build args, an
# optional mirror override, DOCKER_PULL support, git metadata labels, then runs the build.
# Arguments:
#   $1 - name of the bash array variable holding the build command
#   $2 - script_dir (used for git label metadata)
#   $3 - (optional) name of the mirror override env var (e.g., DNF_MIRROR_BASE_URL)
finalize_build() {
    local _cmd_var="$1"
    local script_dir="$2"
    local mirror_var="${3:-}"

    add_proxy_build_args "$1"

    if [[ -n "$mirror_var" ]] && [[ -n "${!mirror_var:-}" ]]; then
        local _mirror_val="${!mirror_var}"
        eval "${_cmd_var}+=(--build-arg \"${mirror_var}=${_mirror_val}\")"
    fi

    if [[ "${DOCKER_PULL:-true}" != "false" ]]; then
        eval "${_cmd_var}+=(--pull)"
    fi

    if command -v git >/dev/null \
        && git -C "$script_dir" rev-parse --is-inside-work-tree >/dev/null 2>&1
    then
        local revision
        revision="$(git -C "$script_dir" rev-parse HEAD 2>/dev/null)"
        eval "${_cmd_var}+=(--label \"org.opencontainers.image.revision=${revision}\")"

        local remote_url
        if remote_url="$(git -C "$script_dir" remote get-url origin 2>/dev/null)"; then
            eval "${_cmd_var}+=(--label \"org.opencontainers.image.source=${remote_url}\")"
        fi
    fi

    local -a _build_cmd
    eval "_build_cmd=(\"\${${_cmd_var}[@]}\")"
    echo "Running: ${_build_cmd[*]}"
    "${_build_cmd[@]}"
}

# Appends --build-arg flags for proxy environment variables to the given build command array. Also
# handles Docker network mode:
#   - If DOCKER_NETWORK is set, uses that value (e.g., DOCKER_NETWORK=host)
#   - Otherwise, if any proxy URL points to localhost/127.0.0.1, defaults to --network host so the
#     build container can reach the host's proxy
#   - Otherwise, uses Docker's default (bridge) network
# Arguments:
#   $1 - name of the bash array variable holding the build command
add_proxy_build_args() {
    local _cmd_var="$1"

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
            local _val="${!var}"
            eval "${_cmd_var}+=(--build-arg \"${var}=${_val}\")"
            if [[ "${_val}" =~ (://localhost[:/]|://127\.0\.0\.1[:/]|://\[::1\][:/]) ]]; then
                has_localhost_proxy=true
            fi
        fi
    done
    for var in "${no_proxy_vars[@]}"; do
        if [[ -n "${!var:-}" ]]; then
            local _val="${!var}"
            eval "${_cmd_var}+=(--build-arg \"${var}=${_val}\")"
        fi
    done

    if [[ -n "${DOCKER_NETWORK:-}" ]]; then
        eval "${_cmd_var}+=(--network \"${DOCKER_NETWORK}\")"
    elif [[ "$has_localhost_proxy" == "true" ]]; then
        eval "${_cmd_var}+=(--network host)"
    fi
}
