# Single-container image

The single-container image runs the CLP package plus its bundled MariaDB, RabbitMQ, Redis, and
MongoDB services in one Docker container. It is intended for local evaluation, demos, and
single-host testing. For production or multi-host deployments, use
[Docker Compose](guides-docker-compose-deployment.md) or [Kubernetes](guides-k8s-deployment.md).

The examples below publish only CLP-facing ports:

* WebUI: `4000`
* API server: `3001`
* log-ingestor: `3002`

MariaDB, RabbitMQ, Redis, and MongoDB bind to loopback inside the container and are not published to
the host.

## Prepare configuration

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

Mount the credentials file at `/etc/clp-credentials.yaml` and set
`CLP_SINGLE_CONTAINER_CREDENTIALS_FILE=/etc/clp-credentials.yaml`. The entrypoint rewrites the
runtime config inside the container to point at that file. Do not put passwords in Docker command
line flags unless you are deliberately overriding the credentials file for a local test.

If `etc/credentials.yaml` does not exist yet, copy `etc/credentials.template.yaml` and uncomment or
set the database, queue, and Redis credentials.

## Start

Build or pull the image, then start it with a config file and credentials file:

```shell
mkdir -p build/clp-package/var/data build/clp-package/var/log build/clp-package/var/tmp

docker run \
  --detach \
  --log-driver local \
  --name clp-package-single \
  --network bridge \
  --restart on-failure:3 \
  --workdir /opt/clp \
  --mount type=bind,src="$(realpath build/clp-package/etc/clp-config.yaml)",dst=/etc/clp-config.yaml,readonly \
  --mount type=bind,src="$(realpath build/clp-package/etc/credentials.yaml)",dst=/etc/clp-credentials.yaml,readonly \
  --mount type=bind,src="$(realpath build/clp-package/var/data)",dst=/var/data \
  --mount type=bind,src="$(realpath build/clp-package/var/log)",dst=/var/log \
  --mount type=bind,src="$(realpath build/clp-package/var/tmp)",dst=/var/tmp \
  --mount type=bind,src=/home/alice/logs,dst=/mnt/logs/home/alice/logs,readonly \
  --env CLP_SINGLE_CONTAINER_CREDENTIALS_FILE=/etc/clp-credentials.yaml \
  --env CLP_SINGLE_CONTAINER_INTERNAL_BIND_ADDRESS=127.0.0.1 \
  --publish 127.0.0.1:4000:4000 \
  --publish 127.0.0.1:3001:3001 \
  --publish 127.0.0.1:3002:3002 \
  clp-package-single:latest
```

The WebUI will be available at <http://127.0.0.1:4000>.

To use non-default host ports, change only the host side of the publish flags:

```shell
--publish 127.0.0.1:4100:4000
--publish 127.0.0.1:3101:3001
--publish 127.0.0.1:3102:3002
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
