#!/usr/bin/env bash

# Points apt at an alternate mirror when APT_MIRROR_URL is set, e.g. an
# organization-internal or regional one. A no-op otherwise.
#
# Handles both x86 (archive.ubuntu.com/ubuntu) and ARM (ports.ubuntu.com/ubuntu-ports).
#
# Usage: APT_MIRROR_URL=<url> ./build.sh

set -o errexit
set -o nounset
set -o pipefail

# shellcheck source=components/core/tools/scripts/lib_install/ca-trust-pkg-opts.sh
source "$(dirname "${BASH_SOURCE[0]}")/../ca-trust-pkg-opts.sh"

if [ -z "${APT_MIRROR_URL:-}" ]; then
    exit 0
fi

# Escape the replacement text before it reaches sed: an unescaped `&` expands to the
# whole match, and a `\` or the `|` delimiter would corrupt the expression.
mirror_url="$(printf '%s' "${APT_MIRROR_URL}" | sed -e 's/[\\&|]/\\&/g')"

sed -i \
    -e "s|https\?://ports.ubuntu.com/ubuntu-ports|${mirror_url}|g" \
    -e "s|https\?://archive.ubuntu.com/ubuntu|${mirror_url}|g" \
    -e "s|https\?://security.ubuntu.com/ubuntu|${mirror_url}|g" \
    /etc/apt/sources.list

# Refresh here so a bad mirror fails at this step rather than midway through
# package installation.
apt-get "${CLP_APT_CA_OPTS[@]}" update
