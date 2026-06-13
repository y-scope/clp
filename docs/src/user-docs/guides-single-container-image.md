# Single-container image

The single-container image runs the CLP package plus its bundled MariaDB, RabbitMQ, Redis, and
MongoDB services in one Docker container. It is intended for local evaluation, demos, and
single-host testing. For production or multi-host deployments, use
[Docker Compose](guides-docker-compose-deployment.md) or [Kubernetes](guides-k8s-deployment.md).

For a first run, use the single-container tabs in the [`clp-json` quick-start][clp-json] or
[`clp-text` quick-start][clp-text]. This page covers custom runtime settings and operational
details for the image.

The examples below publish only the WebUI port. MariaDB, RabbitMQ, Redis, and MongoDB bind to
loopback inside the container and are not published to the host.

## Custom configuration

Use the same `clp-config.yaml` and `credentials.yaml` format as the Docker Compose package. For
filesystem ingestion, set `logs_input.directory` to the absolute host directory you want CLP to
read.

For example:

```yaml
logs_input:
  type: "fs"
  directory: "/home/alice/logs"
```

Keep `logs_input.directory` as the host path. When CLP starts inside the container, it transforms
that path under `/mnt/logs`, so the matching Docker mount for `/home/alice/logs` is
`/mnt/logs/home/alice/logs`.

Mount the credentials file at `/opt/clp/etc/credentials.yaml`, which matches the default
`credentials_file_path` in the package config. Do not put passwords in Docker command line flags
unless you are deliberately overriding the credentials file for a local test.

If `etc/credentials.yaml` does not exist yet, copy `etc/credentials.template.yaml` and uncomment or
set the database, queue, and Redis credentials.

## Custom Docker run

Build or pull the image, then start it with explicit config, credentials, and filesystem-ingestion
mounts:

```shell
export CLP_PACKAGE_DIR="${CLP_PACKAGE_DIR:-$PWD}"
export CLP_LOGS_INPUT_DIR="/home/alice/logs"

docker run \
  --detach \
  --name clp-package-single \
  --mount "type=bind,src=$(realpath "${CLP_PACKAGE_DIR}/etc/clp-config.yaml"),dst=/etc/clp-config.yaml,readonly" \
  --mount "type=bind,src=$(realpath "${CLP_PACKAGE_DIR}/etc/credentials.yaml"),dst=/opt/clp/etc/credentials.yaml,readonly" \
  --mount "type=bind,src=${CLP_LOGS_INPUT_DIR},dst=/mnt/logs${CLP_LOGS_INPUT_DIR},readonly" \
  --publish 127.0.0.1:4000:4000 \
  clp-package-single:latest
```

The WebUI will be available at <http://127.0.0.1:4000>.

To persist data, logs, and temporary files outside the container, create the directories and add the
mounts to the `docker run` command:

```shell
mkdir -p "${CLP_PACKAGE_DIR}/var/data" "${CLP_PACKAGE_DIR}/var/log" "${CLP_PACKAGE_DIR}/var/tmp"

--mount "type=bind,src=$(realpath "${CLP_PACKAGE_DIR}/var/data"),dst=/var/data"
--mount "type=bind,src=$(realpath "${CLP_PACKAGE_DIR}/var/log"),dst=/var/log"
--mount "type=bind,src=$(realpath "${CLP_PACKAGE_DIR}/var/tmp"),dst=/var/tmp"
```

To access the API server or log-ingestor directly from the host, publish their ports explicitly:

```shell
--publish 127.0.0.1:3001:3001
--publish 127.0.0.1:3002:3002
```

To use non-default host ports, change only the host side of the publish flags:

```shell
--publish 127.0.0.1:4100:4000
```

## MCP server

The MCP server is included but disabled by default. To run it without publishing a host port, add:

```shell
--env CLP_MCP_SERVER_ENABLED=1
```

To publish it on localhost, add both flags:

```shell
--env CLP_MCP_SERVER_ENABLED=1
--publish 127.0.0.1:8000:8000
```

## Stop and inspect

```shell
docker ps --filter name=clp-package-single
docker rm --force clp-package-single
```

Service logs are written under the mounted `/var/log` directory.

[clp-json]: quick-start/clp-json.md
[clp-text]: quick-start/clp-text.md
