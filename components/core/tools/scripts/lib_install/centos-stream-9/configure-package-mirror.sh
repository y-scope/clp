#!/usr/bin/env bash

# Points dnf at an alternate mirror when DNF_MIRROR_BASE_URL is set, e.g. an
# organization-internal or regional one. A no-op otherwise.
#
# Usage: DNF_MIRROR_BASE_URL=<url> ./build.sh

set -o errexit
set -o nounset
set -o pipefail

# shellcheck source=components/core/tools/scripts/lib_install/ca-trust-pkg-opts.sh
source "$(dirname "${BASH_SOURCE[0]}")/../ca-trust-pkg-opts.sh"

if [ -z "${DNF_MIRROR_BASE_URL:-}" ]; then
    exit 0
fi

# Escape the replacement text before it reaches sed: an unescaped `&` expands to the
# whole match, and a `\` or the `|` delimiter would corrupt the expression.
mirror_url="$(printf '%s' "${DNF_MIRROR_BASE_URL}" | sed -e 's/[\\&|]/\\&/g')"

sed -i 's|^metalink=|#metalink=|g' /etc/yum.repos.d/centos.repo
sed -i '/^\[baseos\]$/a baseurl='"${mirror_url}"'/9-stream/BaseOS/$basearch/os/' \
    /etc/yum.repos.d/centos.repo
sed -i '/^\[appstream\]$/a baseurl='"${mirror_url}"'/9-stream/AppStream/$basearch/os/' \
    /etc/yum.repos.d/centos.repo
sed -i '/^\[crb\]$/a baseurl='"${mirror_url}"'/9-stream/CRB/$basearch/os/' \
    /etc/yum.repos.d/centos.repo

# Refresh here so a bad mirror fails at this step rather than midway through
# package installation.
dnf clean all
dnf "${CLP_DNF_CA_OPTS[@]}" makecache
