#!/bin/bash

# Exit on any error
set -e

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
component_root="$script_dir/../../../"

# Run the user's command in the container, relative to the root of this
# component
container_component_root=/mnt/clp
docker run \
  -i \
  --rm \
  -u"$(id -u):$(id -g)" \
  --mount "type=bind,src=$(readlink -f "$component_root"),dst=$container_component_root" \
  -w "$container_component_root" \
  ghcr.io/y-scope/clp/clp-core-dependencies-x86-ubuntu-focal:main "$@"
