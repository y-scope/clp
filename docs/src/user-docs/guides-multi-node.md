# Multi-node deployment

A multi-node deployment allows you to run CLP across a distributed set of hosts.

:::{warning}
The instructions below provide a temporary solution for multi-node deployment using manual Docker
Compose orchestration and may change as we actively work to improve ease of deployment. Stay tuned
for future updates on Kubernetes Helm support, which will simplify multi-node deployments
significantly.
:::

## Requirements

* [Docker] and [Docker Compose][docker-compose]
  * If you're not running as root, ensure Docker can be run
    [without superuser privileges][docker-non-root].
* One or more hosts networked together
* When not using S3 storage, a shared filesystem accessible by all worker hosts (e.g., NFS,
  [SeaweedFS])
  * See [below](#setting-up-seaweedfs) for how to set up a simple SeaweedFS cluster.

## Cluster overview

The CLP package is composed of several services that can be categorized as infrastructure services,
schedulers, workers, and supporting services. For a detailed overview of all services and their
dependencies, see the [deployment orchestration design doc][design-orchestration].

:::{note}
Multi-node deployment currently requires manual orchestration of Docker Compose services across your
cluster. This guide explains how to use the `--setup-only` flag and Docker Compose to deploy CLP
components across multiple hosts.
:::

In a multi-node cluster:
* **Infrastructure services** and **schedulers** should be run once per cluster (singleton services)
* **Workers** can be run on multiple hosts to increase parallelism

## Configuring CLP

1. **Extract the CLP package** on one host (the "setup host") where you will prepare the
   configuration.

2. **Configure credentials**:
   * Copy `etc/credentials.template.yml` to `etc/credentials.yml`.
   * Edit `etc/credentials.yml` to set usernames and passwords.

3. **Configure CLP settings**:
   * Edit `etc/clp-config.yml`.
   * Set the `host` and `port` fields for each service to the actual hostname/IP and port where you
     plan to run them.
   * When not using S3 storage (i.e., local filesystem storage), set `logs_input.storage.directory`,
     `archive_output.storage.directory` and `stream_output.storage.directory` to directories on the
     shared filesystem.

4. **Initialize the package environment**:

   Run the following command on the setup host:

   ```bash
   sbin/start-clp.sh --setup-only
   ```

   This will:
   * Validate your configuration
   * Create the necessary directories
   * Generate the `.env` file with all environment variables
   * Create `var/log/.clp-config.yml` (the container-specific config)

5. **Customize the generated configuration for multi-node deployment**:

   After running `--setup-only`, the generated configuration files will have hostnames converted to
   Docker Compose service names (e.g., `database`, `queue`) and ports reset to defaults. For
   multi-node deployment, you **must** update these files to use actual network-reachable
   hostnames/IPs:

   1. **Edit `var/log/.clp-config.yml`**:
      * Update all `host` fields to use the actual hostname or IP address where each service will
        run (matching what you configured in `etc/clp-config.yml`).
      * Verify that ports match your configuration.
      * For example, if your database runs on `192.168.1.10:3306`, ensure `database.host` is set to
        `192.168.1.10` and `database.port` is `3306`.

   2. **Edit `var/www/webui/server/dist/settings.json`**:
      * Update `SqlDbHost` to the actual hostname or IP address of your database service.
      * Update `SqlDbPort` if you changed the database port.
      * Update `MongoDbHost` to the actual hostname or IP address of your results cache service.
      * Update `MongoDbPort` if you changed the results cache port.

   3. **Edit `.env` file** (if needed):
      * Update any host-specific paths or settings.

   :::{important}
   The transformation to service names is intended for single-node Docker Compose deployments. For
   multi-node deployments, services on different hosts need to communicate via actual network
   addresses, not Docker service names.
   :::

6. **Distribute the package** to all hosts where you want to run CLP components:

   Copy the entire CLP package directory (including the generated `.env`,
   `var/log/.clp-config.yml`, and `var/log/instance-id` files) to all hosts in your cluster.

7. **Configure worker concurrency** (optional):

   On each worker host, edit the `.env` file to adjust worker concurrency settings as needed:
   * `CLP_COMPRESSION_WORKER_CONCURRENCY`
   * `CLP_QUERY_WORKER_CONCURRENCY`
   * `CLP_REDUCER_CONCURRENCY`

   Recommended settings:
   * If workers are started on separate hosts, set each concurrency value to match the CPU count on
     that host.
   * If compression and query/reducer workers are started on the same host, set each concurrency
     value to half the CPU count (e.g., for a 16-core host: set all three to 8).

## Starting CLP

Start the services using the commands below. The comments indicate the startup order and
dependencies.

:::{note}
For **clp-json + Presto** deployments (storage engine: `clp-s` with query engine: `presto`), omit
the `query-scheduler`, `query-worker`, and `reducer` services.
:::

:::{tip}
If you want to use your own MariaDB/MySQL or MongoDB servers instead of the Docker Compose managed
databases, see the [external database setup reference](reference-external-database.md) for
instructions. When using external databases, skip starting the `database` and `results-cache`
services below.
:::

All commands below assume you are running them from the root of the CLP package directory.

```bash
################################################################################
# Infrastructure services
################################################################################

# Start database (skip if using external database)
docker compose --project-name "clp-package-$(cat var/log/instance-id)" \
  --file docker-compose-all.yaml \
  up database --no-deps --detach --wait

# Initialize database
docker compose --project-name "clp-package-$(cat var/log/instance-id)" \
  --file docker-compose-all.yaml \
  up db-table-creator --no-deps

# Start queue
docker compose --project-name "clp-package-$(cat var/log/instance-id)" \
  --file docker-compose-all.yaml \
  up queue --no-deps --detach --wait

# Start redis
docker compose --project-name "clp-package-$(cat var/log/instance-id)" \
  --file docker-compose-all.yaml \
  up redis --no-deps --detach --wait

# Start results cache (skip if using external MongoDB)
docker compose --project-name "clp-package-$(cat var/log/instance-id)" \
  --file docker-compose-all.yaml \
  up results-cache --no-deps --detach --wait

# Initialize results cache
docker compose --project-name "clp-package-$(cat var/log/instance-id)" \
  --file docker-compose-all.yaml \
  up results-cache-indices-creator --no-deps

################################################################################
# Controller services (schedulers, UI, and supporting services)
################################################################################

# Start compression scheduler
docker compose --project-name "clp-package-$(cat var/log/instance-id)" \
  --file docker-compose-all.yaml \
  up compression-scheduler --no-deps --detach

# Start query scheduler
docker compose --project-name "clp-package-$(cat var/log/instance-id)" \
  --file docker-compose-all.yaml \
  up query-scheduler --no-deps --detach --wait

# Start webui
docker compose --project-name "clp-package-$(cat var/log/instance-id)" \
  --file docker-compose-all.yaml \
  up webui --no-deps --detach

# Start garbage collector (optional, only if retention is configured)
docker compose --project-name "clp-package-$(cat var/log/instance-id)" \
  --file docker-compose-all.yaml \
  up garbage-collector --no-deps --detach

# Start MCP server (optional, only if configured)
docker compose --project-name "clp-package-$(cat var/log/instance-id)" \
  --file docker-compose-all.yaml \
  up mcp-server --no-deps --detach

################################################################################
# Worker services (can be started on multiple hosts)
################################################################################

# Start compression worker
docker compose --project-name "clp-package-$(cat var/log/instance-id)" \
  --file docker-compose-all.yaml \
  up compression-worker --no-deps --detach

# Start query worker
docker compose --project-name "clp-package-$(cat var/log/instance-id)" \
  --file docker-compose-all.yaml \
  up query-worker --no-deps --detach

# Start reducer
docker compose --project-name "clp-package-$(cat var/log/instance-id)" \
  --file docker-compose-all.yaml \
  up reducer --no-deps --detach
```

:::{note}
To increase parallelism, start worker services (`compression-worker`, `query-worker`, `reducer`) on
multiple hosts. Ensure the shared filesystem is mounted at the same path on all worker hosts, then
run the worker commands on each host.
:::

## Using CLP

To learn how to compress and search your logs, check out the quick-start guide that corresponds to
the flavor of CLP you're running:

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: quick-start/clp-json
Using clp-json
^^^
How to compress and search JSON logs.
:::

:::{grid-item-card}
:link: quick-start/clp-text
Using clp-text
^^^
How to compress and search unstructured text logs.
:::
::::

## Stopping CLP

To stop CLP, run:

```bash
sbin/stop-clp.sh
```

This will stop all CLP services managed by Docker Compose on the current host.

## Monitoring services

To check the status of services on a host:

```bash
docker compose --project-name clp-package-<instance-id> ps
```

To view logs for a specific service:

```bash
docker compose --project-name clp-package-<instance-id> logs -f <service-name>
```

## Setting up SeaweedFS

The instructions below are for running a simple SeaweedFS cluster on a set of hosts. For other use
cases, see the [SeaweedFS docs][seaweedfs-docs].

1. Install [SeaweedFS][seaweedfs-install-docs].
2. Start the master and a filer on one of the hosts:

    ```bash
    weed master -port 9333
    weed filer -port 8888 -master "localhost:9333"
    ```

3. Start one or more volume servers on one or more hosts.

    {style=lower-alpha}
    1. Create a directory where you want SeaweedFS to store data.
    2. Start the volume server:

        ```bash
        weed volume -mserver "<master-host>:9333" -dir <storage-dir> -max 0
        ```

        * `<master-host>` is the hostname/IP of the master host.
        * `<storage-dir>` is the directory where you want SeaweedFS to store data.
4. Start a FUSE mount on every host that you want to run a CLP worker:

     ```bash
     weed mount -filer "<master-host>:8888" -dir <mount-path>
     ```

     * `<master-host>` is the hostname/IP of the master host.
     * `<mount-path>` is the path where you want the mount to be.

[design-orchestration]: ../dev-docs/design-deployment-orchestration.md
[Docker]: https://docs.docker.com/engine/install/
[docker-compose]: https://docs.docker.com/compose/
[docker-non-root]: https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user
[SeaweedFS]: https://github.com/seaweedfs/seaweedfs
[seaweedfs-docs]: https://github.com/seaweedfs/seaweedfs/blob/master/README.md
[seaweedfs-install-docs]: https://github.com/seaweedfs/seaweedfs?tab=readme-ov-file#quick-start-with-single-binary
