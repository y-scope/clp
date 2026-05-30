#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

get_package_version() {
    local package="$1"
    dpkg-query -W -f='${Version}' "$package" 2>/dev/null
}

strip_epoch() {
    local version="$1"
    echo "${version#*:}"
}

require_package() {
    local package="$1"
    local status=""
    status="$(dpkg-query -W -f='${db:Status-Abbrev}' "$package" 2>/dev/null || true)"
    if [[ "$status" != ii* ]]; then
        echo >&2 "Error: Required package '$package' is not installed."
        exit 1
    fi
}

for package in mariadb-server mongodb-org-server rabbitmq-server redis-server supervisor; do
    require_package "$package"
done

redis_version="$(strip_epoch "$(get_package_version redis-server)")"
if dpkg --compare-versions "$redis_version" ge "7.4"; then
    echo >&2 \
        "Error: redis-server $redis_version is not allowed in the single-container image. Use < 7.4."
    exit 1
fi

mongodb_copyright="/usr/share/doc/mongodb-org-server/copyright"
if [[ ! -f "$mongodb_copyright" ]] || ! grep -qi "SSPL" "$mongodb_copyright"; then
    echo >&2 "Error: MongoDB server license notice is missing or does not mention SSPL."
    exit 1
fi

mariadb_copyright="/usr/share/doc/mariadb-server/copyright"
if [[ ! -f "$mariadb_copyright" ]] || ! grep -qi "GPL-2" "$mariadb_copyright"; then
    echo >&2 "Error: MariaDB server GPL-2 license notice is missing."
    exit 1
fi

for build_only_package in curl gnupg; do
    status="$(dpkg-query -W -f='${db:Status-Abbrev}' "$build_only_package" 2>/dev/null || true)"
    if [[ "$status" == ii* ]]; then
        echo >&2 "Error: Build-only package '$build_only_package' is still installed."
        exit 1
    fi
done
