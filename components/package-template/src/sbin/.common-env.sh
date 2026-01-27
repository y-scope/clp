#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
package_root=$(readlink -f "$script_dir/..")

if [[ -z "${CLP_HOME:-}" ]]; then
    export CLP_HOME="$package_root"
else
    export CLP_HOME="$CLP_HOME"
fi

image_id_file="$CLP_HOME/clp-package-image.id"
version_file="$CLP_HOME/VERSION"

if [[ -f "$image_id_file" ]]; then
    image_id="$(tr -d '[:space:]' <"$image_id_file")"
    export CLP_PACKAGE_CONTAINER_IMAGE_REF="$image_id"
elif [[ -f "$version_file" ]]; then
    version="$(tr -d '[:space:]' <"$version_file")"
    export CLP_PACKAGE_CONTAINER_IMAGE_REF="ghcr.io/y-scope/clp/clp-package:$version"
else
    echo >&2 "Error: Neither '${image_id_file}' nor '${version_file}' exist."
    return 1
fi

uid="$(id --user 2>/dev/null || echo "1000")"
gid="$(getent group docker | cut -d: -f3 2>/dev/null || echo "999")"
export CLP_FIRST_PARTY_SERVICE_UID_GID="$uid:$gid"

CLP_PWD_HOST="$(pwd 2>/dev/null || echo "")"
export CLP_PWD_HOST

if [[ -z "${CLP_DOCKER_PLUGIN_DIR:-}" ]]; then
    compose_plugin_path="$(docker info \
        --format '{{range .ClientInfo.Plugins}}{{if eq .Name "compose"}}{{.Path}}{{end}}{{end}}' \
        2>/dev/null)"

    if [[ -z "$compose_plugin_path" || ! -f "$compose_plugin_path" ]]; then
        echo >&2 "Error: Docker Compose plugin not found via 'docker info'."
        return 1
    fi

    resolved_plugin_path="$(readlink -f "$compose_plugin_path" 2>/dev/null || true)"
    if [[ -z "$resolved_plugin_path" ]]; then
        echo >&2 "Error: Failed to resolve Docker Compose plugin's real path."
        return 1
    fi

    plugin_dir="$(dirname "$resolved_plugin_path")"
    if [[ -z "$plugin_dir" || "$plugin_dir" == "." || ! -d "$plugin_dir" ]]; then
        echo >&2 "Error: Failed to resolve Docker Compose plugin directory."
        return 1
    fi

    export CLP_DOCKER_PLUGIN_DIR="$plugin_dir"
fi

if [[ -z "${CLP_DOCKER_SOCK_PATH:-}" ]]; then
    socket="$(docker context inspect \
        --format '{{.Endpoints.docker.Host}}' 2>/dev/null |
        sed -E 's|^unix://||')"

    if [[ -S "$socket" ]]; then
        export CLP_DOCKER_SOCK_PATH="$socket"
    fi
fi

CLP_COMPOSE_RUN_EXTRA_FLAGS=()
if [[ $- != *i* ]]; then
    CLP_COMPOSE_RUN_EXTRA_FLAGS+=(--interactive=false)
fi
