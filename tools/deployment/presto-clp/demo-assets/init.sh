#!/usr/bin/env bash

SCRIPT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

function generate_sample_clp_config {
    local ip=$(hostname -i)
    local file="${SCRIPT_PATH}/clp-config.yml"
    cp "$file" "${file}.bak"
    sed -i "s|\${REPLACE_IP}|$ip|g" "$file"
    echo "Replaced \${REPLACE_IP} with $ip in $file"
}

function update_metadata_config {
    if [[ $# -ne 1 ]]; then
        echo "Usage: update_metadata_config </path/to/clp-package>"
        return 1
    fi
 
    local clp_pkg_home=$1
    local clp_config_path="$(readlink -f ${clp_pkg_home})/etc/clp-config.yml"
    local credential_path="$(readlink -f ${clp_pkg_home})/etc/credentials.yml"
    host=$(python3 -c 'import sys, yaml; print(yaml.load(sys.stdin)["database"]["host"])' < "$clp_config_path")
    port=$(python3 -c 'import sys, yaml; print(yaml.load(sys.stdin)["database"]["port"])' < "$clp_config_path")
    name=$(python3 -c 'import sys, yaml; print(yaml.load(sys.stdin)["database"]["name"])' < "$clp_config_path")
    user=$(python3 -c 'import sys, yaml; print(yaml.load(sys.stdin)["database"]["user"])' < "$credential_path")
    password=$(python3 -c 'import sys, yaml; print(yaml.load(sys.stdin)["database"]["password"])' < "$credential_path")
    echo "Metadata database host: $host"
    echo "Metadata database port: $port"
    echo "Metadata database name: $name"
    echo "Metadata database user: $user"
    echo "Metadata database password: $password"
    
    local env_path="${SCRIPT_PATH}/../.env"
    cp "$env_path" "${env_path}.bak"
    sed -i "s|^PRESTO_COORDINATOR_CONFIG_CLPPROPERTIES_METADATA_DATABASE_URL=.*|PRESTO_COORDINATOR_CONFIG_CLPPROPERTIES_METADATA_DATABASE_URL=\"jdbc:mysql://${host}:${port}\"|" "$env_path"
    sed -i "s/^PRESTO_COORDINATOR_CONFIG_CLPPROPERTIES_METADATA_DATABASE_NAME=.*/PRESTO_COORDINATOR_CONFIG_CLPPROPERTIES_METADATA_DATABASE_NAME=\"${name}\"/" "$env_path"
    sed -i "s/^PRESTO_COORDINATOR_CONFIG_CLPPROPERTIES_METADATA_DATABASE_USER=.*/PRESTO_COORDINATOR_CONFIG_CLPPROPERTIES_METADATA_DATABASE_USER=\"${user}\"/" "$env_path"
    sed -i "s/^PRESTO_COORDINATOR_CONFIG_CLPPROPERTIES_METADATA_DATABASE_PASSWORD=.*/PRESTO_COORDINATOR_CONFIG_CLPPROPERTIES_METADATA_DATABASE_PASSWORD=\"${password}\"/" "$env_path"
    sed -i "s|^CLP_PACKAGE_ARCHIVES=.*|CLP_PACKAGE_ARCHIVES=\"$(readlink -f ${clp_pkg_home})/var/data/archives/default\"|" "$env_path"
}

if declare -f "$1" > /dev/null; then
    "$@"
else
    echo "Error: '$1' is not a valid function name."
    echo "Available functions:"
    declare -F | awk '{print $3}'
    exit 1
fi

