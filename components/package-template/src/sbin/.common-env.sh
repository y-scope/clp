#!/usr/bin/env bash

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
package_root=$(readlink -f "$script_dir/..")

if [[ -z "${CLP_HOME:-}" ]]; then
    export CLP_HOME="$package_root"
else
    export CLP_HOME="$CLP_HOME"
fi

image_id_file="$CLP_HOME/clp-package-image.id"
version_file="$CLP_HOME/VERSION"

if [[ -f "$image_id_file" ]]; then
    image_id="$(tr -d '[:space:]' < "$image_id_file")"
    export CLP_PACKAGE_CONTAINER_IMAGE_REF="$image_id"
elif [[ -f "$version_file" ]]; then
    version="$(tr -d '[:space:]' < "$version_file")"
    export CLP_PACKAGE_CONTAINER_IMAGE_REF="ghcr.io/y-scope/clp/clp-package:v$version"
else
    echo "Error: Neither clp-package-image.id nor VERSION file exists." >&2
    return 1 2>/dev/null || exit 1
fi

uid="$(id --user 2>/dev/null || echo "1000")"
gid="$(getent group docker | cut -d: -f3 2>/dev/null || echo "999")"
export CLP_FIRST_PARTY_SERVICE_UID_GID="$uid:$gid"

CLP_PWD_HOST="$(pwd 2>/dev/null || echo "")"
export CLP_PWD_HOST

if [[ -z "${CLP_DOCKER_PLUGIN_DIR:-}" ]]; then
    for dir in \
        "$HOME/.docker/cli-plugins" \
        "/mnt/wsl/docker-desktop/cli-tools/usr/local/lib/docker/cli-plugins" \
        "/usr/local/lib/docker/cli-plugins" \
        "/usr/libexec/docker/cli-plugins"; do

        compose_plugin_path="$dir/docker-compose"
        if [[ -f "$compose_plugin_path" ]]; then
            export CLP_DOCKER_PLUGIN_DIR="$dir"
            break
        fi
    done
    if [[ -z "${CLP_DOCKER_PLUGIN_DIR:-}" ]]; then
        echo "Warning: Docker plugin directory not found; Docker Compose may not work inside container." >&2
    fi
fi

if [[ -z "${CLP_DOCKER_SOCK_PATH:-}" ]]; then
  socket="$(docker context inspect --format '{{.Endpoints.docker.Host}}' 2>/dev/null | sed -E 's|^unix://||')"
  if [[ -S "$socket" ]]; then
      export CLP_DOCKER_SOCK_PATH="$socket"
  fi
fi
