#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

"${script_dir}/install-prebuilt-packages.sh"

# Prepend the pipx bin directory to PATH so pipx-installed build tools take precedence.
# Required by install-packages-from-source.sh, which installs tools into the pre-baked deps images.
# This can be removed once all library installs switch to go-task, which runs outside this script.
# The same rationale applies to all other platforms (e.g., centos9, manylinux, musllinux, macos),
# though the comment is omitted in those scripts.
pipx_bin_dir="$("${script_dir}/../pipx-packages/get-pipx-bin-dir.sh")"
export PATH="${pipx_bin_dir}:${PATH}"
"${script_dir}/install-packages-from-source.sh"
