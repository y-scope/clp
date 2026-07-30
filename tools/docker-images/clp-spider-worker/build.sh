#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
repo_root="${script_dir}/../../.."

. "${script_dir}/../utils.sh"

build_clp_image "clp-spider-worker" "${repo_root}" "${script_dir}/Dockerfile"
