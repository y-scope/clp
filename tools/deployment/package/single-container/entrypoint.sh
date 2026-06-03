#!/usr/bin/env bash
set -euo pipefail

if [[ "${EUID}" -ne 0 ]]; then
    exec "$@"
fi

prepare_runtime_config() {
    export CLP_CONFIG_PATH="${CLP_CONFIG_PATH:-/etc/clp-config.yaml}"
    export CLP_HOME="${CLP_HOME:-/opt/clp}"

    eval "$(python3 <<'PY'
import os
import pathlib
import shlex

import yaml

from clp_py_utils.clp_config import ClpConfig, ClpDbNameType, ClpDbUserType, StorageType
from clp_py_utils.core import read_yaml_config_file


def emit(name, value):
    print(f"export {name}={shlex.quote(str(value))}")


clp_home = pathlib.Path(os.environ.get("CLP_HOME", "/opt/clp"))
input_config_path = pathlib.Path(os.environ["CLP_CONFIG_PATH"])
runtime_config_path = pathlib.Path("/run/clp-single-container/clp-config.yaml")

raw_config = read_yaml_config_file(input_config_path) or {}
clp_config = ClpConfig.model_validate(raw_config)
clp_config.make_config_paths_absolute(clp_home)
clp_config.validate_api_server()

credentials_file_path = clp_config.credentials_file_path
if credentials_file_path.is_file():
    clp_config.credentials_file_path = credentials_file_path
    clp_config.database.load_credentials_from_file(credentials_file_path)
    if clp_config.queue is not None:
        clp_config.queue.load_credentials_from_file(credentials_file_path)
    if clp_config.redis is not None:
        clp_config.redis.load_credentials_from_file(credentials_file_path)

container_config = clp_config.model_copy(deep=True)
container_config.transform_for_container()
runtime_config_path.write_text(yaml.safe_dump(container_config.dump_to_primitive_dict()))

emit("CLP_CONFIG_PATH", runtime_config_path)
emit("CLP_DB_NAME", clp_config.database.names[ClpDbNameType.CLP])
emit("SPIDER_DB_NAME", clp_config.database.names[ClpDbNameType.SPIDER])
emit("CLP_DB_CONNECT_PORT", container_config.database.port)
emit("CLP_QUEUE_CONNECT_PORT", container_config.queue.port if container_config.queue else 5672)
emit("CLP_REDIS_CONNECT_PORT", container_config.redis.port if container_config.redis else 6379)
emit("CLP_REDIS_BACKEND_DB_COMPRESSION", clp_config.redis.compression_backend_database)
emit("CLP_REDIS_BACKEND_DB_QUERY", clp_config.redis.query_backend_database)
emit("CLP_RESULTS_CACHE_CONNECT_PORT", container_config.results_cache.port)
emit("CLP_RESULTS_CACHE_DB_NAME", clp_config.results_cache.db_name)
emit(
    "CLP_RESULTS_CACHE_STREAM_COLLECTION_NAME",
    clp_config.results_cache.stream_collection_name,
)
emit("CLP_PACKAGE_STORAGE_ENGINE", clp_config.package.storage_engine)
emit("CLP_REDUCER_UPSERT_INTERVAL", clp_config.reducer.upsert_interval)
emit("CLP_API_SERVER_ENABLED", int(clp_config.api_server is not None))
emit(
    "CLP_LOG_INGESTOR_ENABLED",
    int(clp_config.log_ingestor is not None and clp_config.logs_input.type == StorageType.S3),
)
emit(
    "CLP_GARBAGE_COLLECTOR_ENABLED",
    int(
        clp_config.archive_output.retention_period is not None
        or clp_config.results_cache.retention_period is not None
    ),
)
emit("CLP_GARBAGE_COLLECTOR_LOGGING_LEVEL", clp_config.garbage_collector.logging_level)
if clp_config.log_ingestor is not None:
    emit("CLP_LOG_INGESTOR_LOGGING_LEVEL", clp_config.log_ingestor.logging_level)
if clp_config.mcp_server is not None:
    emit("CLP_MCP_LOGGING_LEVEL", clp_config.mcp_server.logging_level)

db_credentials = clp_config.database.credentials
if ClpDbUserType.CLP in db_credentials:
    emit("CLP_DB_USER", db_credentials[ClpDbUserType.CLP].username)
    emit("CLP_DB_PASS", db_credentials[ClpDbUserType.CLP].password)
if ClpDbUserType.ROOT in db_credentials:
    emit("CLP_DB_ROOT_USER", db_credentials[ClpDbUserType.ROOT].username)
    emit("CLP_DB_ROOT_PASS", db_credentials[ClpDbUserType.ROOT].password)
if ClpDbUserType.SPIDER in db_credentials:
    emit("SPIDER_DB_USER", db_credentials[ClpDbUserType.SPIDER].username)
    emit("SPIDER_DB_PASS", db_credentials[ClpDbUserType.SPIDER].password)
if clp_config.queue is not None and clp_config.queue.username is not None:
    emit("CLP_QUEUE_USER", clp_config.queue.username)
    emit("CLP_QUEUE_PASS", clp_config.queue.password)
if clp_config.redis is not None and clp_config.redis.password is not None:
    emit("CLP_REDIS_PASS", clp_config.redis.password)
PY
)"
}

configure_service_environment() {
    prepare_runtime_config

    export CLP_DB_NAME="${CLP_DB_NAME:-${MYSQL_DATABASE:-}}"
    export CLP_DB_USER="${CLP_DB_USER:-${MYSQL_USER:-}}"
    export CLP_DB_PASS="${CLP_DB_PASS:-${MYSQL_PASSWORD:-}}"
    export CLP_DB_ROOT_USER="${CLP_DB_ROOT_USER:-root}"
    export CLP_DB_ROOT_PASS="${CLP_DB_ROOT_PASS:-${MYSQL_ROOT_PASSWORD:-}}"
    export CLP_QUEUE_USER="${CLP_QUEUE_USER:-${RABBITMQ_DEFAULT_USER:-}}"
    export CLP_QUEUE_PASS="${CLP_QUEUE_PASS:-${RABBITMQ_DEFAULT_PASS:-}}"
    export CLP_REDIS_PASS="${CLP_REDIS_PASS:-${REDIS_PASSWORD:-}}"

    : "${CLP_DB_NAME:?Please set database.names.clp in CLP config.}"
    : "${CLP_DB_USER:?Please set database.username in credentials or CLP_DB_USER.}"
    : "${CLP_DB_PASS:?Please set CLP_DB_PASS or MYSQL_PASSWORD.}"
    : "${CLP_DB_ROOT_PASS:?Please set CLP_DB_ROOT_PASS or MYSQL_ROOT_PASSWORD.}"
    : "${CLP_QUEUE_USER:?Please set queue.username in credentials or CLP_QUEUE_USER.}"
    : "${CLP_QUEUE_PASS:?Please set CLP_QUEUE_PASS or RABBITMQ_DEFAULT_PASS.}"
    : "${CLP_REDIS_PASS:?Please set CLP_REDIS_PASS or REDIS_PASSWORD.}"

    export MYSQL_DATABASE="${MYSQL_DATABASE:-${CLP_DB_NAME}}"
    export MYSQL_USER="${MYSQL_USER:-${CLP_DB_USER}}"
    export MYSQL_PASSWORD="${MYSQL_PASSWORD:-${CLP_DB_PASS}}"
    export MYSQL_ROOT_PASSWORD="${MYSQL_ROOT_PASSWORD:-${CLP_DB_ROOT_PASS}}"
    export RABBITMQ_DEFAULT_USER="${RABBITMQ_DEFAULT_USER:-${CLP_QUEUE_USER}}"
    export RABBITMQ_DEFAULT_PASS="${RABBITMQ_DEFAULT_PASS:-${CLP_QUEUE_PASS}}"
    export RABBITMQ_LOGS="${RABBITMQ_LOGS:-/var/log/rabbitmq/rabbitmq.log}"
    export REDIS_PASSWORD="${REDIS_PASSWORD:-${CLP_REDIS_PASS}}"

    export CLP_DB_CONNECT_PORT="${CLP_DB_CONNECT_PORT:-3306}"
    export CLP_QUEUE_CONNECT_PORT="${CLP_QUEUE_CONNECT_PORT:-5672}"
    export CLP_REDIS_CONNECT_PORT="${CLP_REDIS_CONNECT_PORT:-6379}"
    export CLP_REDIS_BACKEND_DB_COMPRESSION="${CLP_REDIS_BACKEND_DB_COMPRESSION:-1}"
    export CLP_REDIS_BACKEND_DB_QUERY="${CLP_REDIS_BACKEND_DB_QUERY:-0}"
    export CLP_RESULTS_CACHE_CONNECT_PORT="${CLP_RESULTS_CACHE_CONNECT_PORT:-27017}"
    export CLP_RESULTS_CACHE_DB_NAME="${CLP_RESULTS_CACHE_DB_NAME:-clp-query-results}"
    export CLP_RESULTS_CACHE_STREAM_COLLECTION_NAME="${CLP_RESULTS_CACHE_STREAM_COLLECTION_NAME:-stream-files}"
    export CLP_PACKAGE_STORAGE_ENGINE="${CLP_PACKAGE_STORAGE_ENGINE:-clp}"
    export CLP_COMPRESSION_WORKER_CONCURRENCY="${CLP_COMPRESSION_WORKER_CONCURRENCY:-1}"
    export CLP_QUERY_WORKER_CONCURRENCY="${CLP_QUERY_WORKER_CONCURRENCY:-1}"
    export CLP_REDUCER_CONCURRENCY="${CLP_REDUCER_CONCURRENCY:-1}"
    export CLP_REDUCER_UPSERT_INTERVAL="${CLP_REDUCER_UPSERT_INTERVAL:-100}"
    export CLP_API_SERVER_ENABLED="${CLP_API_SERVER_ENABLED:-1}"
    export CLP_LOG_INGESTOR_ENABLED="${CLP_LOG_INGESTOR_ENABLED:-0}"
    export CLP_LOG_INGESTOR_LOGGING_LEVEL="${CLP_LOG_INGESTOR_LOGGING_LEVEL:-INFO}"
    export CLP_GARBAGE_COLLECTOR_ENABLED="${CLP_GARBAGE_COLLECTOR_ENABLED:-1}"
    export CLP_GARBAGE_COLLECTOR_LOGGING_LEVEL="${CLP_GARBAGE_COLLECTOR_LOGGING_LEVEL:-INFO}"
    export CLP_MCP_SERVER_ENABLED="${CLP_MCP_SERVER_ENABLED:-0}"
    export CLP_MCP_LOGGING_LEVEL="${CLP_MCP_LOGGING_LEVEL:-INFO}"
    export CLP_LOGS_DIR="${CLP_LOGS_DIR:-/var/log}"
    export BROKER_URL="amqp://${CLP_QUEUE_USER:-clp-user}:${CLP_QUEUE_PASS:?Please set CLP_QUEUE_PASS.}@queue:${CLP_QUEUE_CONNECT_PORT}"
    export HOST="${HOST:-0.0.0.0}"
    export NODE_ENV="${NODE_ENV:-production}"
    export NODE_PATH="${NODE_PATH:-/opt/clp/var/www/webui/server/node_modules}"
    export PORT="${PORT:-4000}"
}

initialize_mariadb_data_dir() {
    if [[ -d /var/data/database/mysql ]]; then
        return
    fi

    mariadb-install-db \
        --user=mysql \
        --datadir=/var/data/database \
        --skip-test-db \
        --auth-root-authentication-method=normal \
        >/dev/null

    python3 <<'PY' | mariadbd --user=mysql --datadir=/var/data/database --bootstrap
import os
import sys


def quote_identifier(value):
    if "\0" in value:
        raise ValueError("SQL identifier contains a NUL byte.")
    return f"`{value.replace('`', '``')}`"


def quote_string(value):
    if "\0" in value:
        raise ValueError("SQL string contains a NUL byte.")
    return "'" + value.replace("\\", "\\\\").replace("'", "''") + "'"


values = {
    "MYSQL_DATABASE": os.environ["MYSQL_DATABASE"],
    "MYSQL_USER": os.environ["MYSQL_USER"],
    "MYSQL_PASSWORD": os.environ["MYSQL_PASSWORD"],
    "MYSQL_ROOT_PASSWORD": os.environ["MYSQL_ROOT_PASSWORD"],
}
for name, value in values.items():
    if value == "":
        sys.exit(f"{name} must not be empty.")

database = quote_identifier(values["MYSQL_DATABASE"])
user = quote_string(values["MYSQL_USER"])
password = quote_string(values["MYSQL_PASSWORD"])
root_password = quote_string(values["MYSQL_ROOT_PASSWORD"])

print("FLUSH PRIVILEGES;")
print(f"ALTER USER 'root'@'localhost' IDENTIFIED BY {root_password};")
print(f"CREATE DATABASE IF NOT EXISTS {database};")
print(f"CREATE USER IF NOT EXISTS {user}@'%' IDENTIFIED BY {password};")
print(f"GRANT ALL PRIVILEGES ON {database}.* TO {user}@'%';")
print("FLUSH PRIVILEGES;")
PY
}

configure_rabbitmq_defaults() {
    mkdir -p /etc/rabbitmq/conf.d
    cat > /etc/rabbitmq/conf.d/10-clp-defaults.conf <<EOF
default_user = ${RABBITMQ_DEFAULT_USER}
default_pass = ${RABBITMQ_DEFAULT_PASS}
log.file = ${RABBITMQ_LOGS}
listeners.tcp.1 = 127.0.0.1:5672
EOF
    cat > /etc/rabbitmq/rabbitmq-env.conf <<'EOF'
NODENAME=rabbit@localhost
SERVER_ADDITIONAL_ERL_ARGS="-kernel inet_dist_use_interface {127,0,0,1}"
EOF
    chown rabbitmq:rabbitmq /etc/rabbitmq/conf.d/10-clp-defaults.conf
    chown rabbitmq:rabbitmq /etc/rabbitmq/rabbitmq-env.conf
    chmod 0640 /etc/rabbitmq/conf.d/10-clp-defaults.conf
    chmod 0640 /etc/rabbitmq/rabbitmq-env.conf
}

configure_webui_settings() {
    local client_settings_path="/opt/clp/var/www/webui/client/settings.json"
    local server_settings_path="/opt/clp/var/www/webui/server/dist/settings.json"
    if [[ ! -f "$server_settings_path" ]]; then
        return
    fi

    python3 - "$CLP_CONFIG_PATH" "$server_settings_path" "$client_settings_path" <<'PY'
import pathlib
import sys

from clp_package_utils.webui_settings import update_webui_settings
from clp_py_utils.clp_config import ClpConfig
from clp_py_utils.core import read_yaml_config_file

config_path = pathlib.Path(sys.argv[1])
server_settings_path = pathlib.Path(sys.argv[2])
client_settings_path = pathlib.Path(sys.argv[3])

clp_config = ClpConfig.model_validate(read_yaml_config_file(config_path))
update_webui_settings(
    clp_config=clp_config,
    container_clp_config=clp_config,
    client_settings_json_path=client_settings_path,
    server_settings_json_path=server_settings_path,
    container_webui_dir=pathlib.Path("/opt/clp/var/www/webui"),
)
PY
}

add_local_alias() {
    local hostname="$1"
    if ! awk -v hostname="$hostname" '
        $1 == "127.0.0.1" {
            for (i = 2; i <= NF; ++i) {
                if ($i == hostname) {
                    found = 1
                }
            }
        }
        END { exit found ? 0 : 1 }
    ' /etc/hosts; then
        echo "127.0.0.1 ${hostname}" >> /etc/hosts
    fi
}

for hostname in database queue redis results_cache compression_scheduler query_scheduler reducer mcp_server; do
    add_local_alias "$hostname"
done

mkdir -p \
    /data/db \
    /run/clp-single-container \
    /run/mysqld \
    /opt/clp/var/data \
    /var/data/archives \
    /var/data/database \
    /var/data/redis \
    /var/data/results_cache \
    /var/data/streams \
    /var/lib/rabbitmq \
    /var/log/api_server \
    /var/log/clp-single-container \
    /var/log/compression_scheduler \
    /var/log/compression_worker \
    /var/log/garbage_collector \
    /var/log/log_ingestor \
    /var/log/mcp_server \
    /var/log/mysql \
    /var/log/mongodb \
    /var/log/query_scheduler \
    /var/log/query_worker \
    /var/log/rabbitmq \
    /var/log/reducer \
    /var/log/redis \
    /var/log/supervisor \
    /var/log/webui

chmod 0755 /var/data /var/log
chmod 1777 /var/tmp
if [[ ! -e /opt/clp/var/tmp ]]; then
    ln -s /var/tmp /opt/clp/var/tmp
fi

chown clp-user:clp-user /var/data /var/log
chown -R mysql:mysql /run/mysqld /var/data/database /var/log/mysql
chown -R mongodb:mongodb /var/data/results_cache /var/log/mongodb
chown -R rabbitmq:rabbitmq /var/lib/rabbitmq /var/log/rabbitmq
chown -R redis:redis /var/data/redis /var/log/redis
chown -R clp-user:clp-user \
    /run/clp-single-container \
    /opt/clp/var/data \
    /var/data/archives \
    /var/data/streams \
    /var/log/api_server \
    /var/log/compression_scheduler \
    /var/log/compression_worker \
    /var/log/garbage_collector \
    /var/log/log_ingestor \
    /var/log/mcp_server \
    /var/log/query_scheduler \
    /var/log/query_worker \
    /var/log/reducer \
    /var/log/webui
if [[ "${1:-}" == */supervisord || "${1:-}" == "supervisord" ]]; then
    configure_service_environment
    initialize_mariadb_data_dir
    configure_rabbitmq_defaults
    configure_webui_settings
fi

exec "$@"
