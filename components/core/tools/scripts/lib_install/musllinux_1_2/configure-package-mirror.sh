#!/usr/bin/env bash

# Points apk at an alternate mirror when APK_MIRROR_URL is set, e.g. an
# organization-internal or regional one. A no-op otherwise.
#
# Usage: APK_MIRROR_URL=<url> ./build.sh

set -o errexit
set -o nounset
set -o pipefail

if [ -z "${APK_MIRROR_URL:-}" ]; then
    exit 0
fi

# Escape the replacement text before it reaches sed: an unescaped `&` expands to the
# whole match, and a `\` or the `|` delimiter would corrupt the expression.
mirror_url="$(printf '%s' "${APK_MIRROR_URL}" | sed -e 's/[\\&|]/\\&/g')"

sed -i "s|https://dl-cdn.alpinelinux.org/alpine|${mirror_url}|g" /etc/apk/repositories

# Refresh here so a bad mirror fails at this step rather than midway through
# package installation.
apk update
