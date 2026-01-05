# Multi-host deployment

A multi-host deployment allows you to run CLP across a distributed set of hosts.

:::{warning}
The instructions below provide a temporary solution for multi-host deployment and may change as we
actively work to improve ease of deployment. The present solution uses *manual* Docker Compose
orchestration; however, Kubernetes Helm support will be available in a future release, which will
simplify multi-host deployments significantly.
:::

## Requirements

* [Docker] and [Docker Compose][docker-compose]
  * If you're not running as root, ensure Docker can be run [without superuser
    privileges][docker-non-root].
* One or more hosts networked together
* When not using S3 storage, a shared filesystem accessible by all worker hosts (e.g., NFS,
  [SeaweedFS])
  * See [below](#setting-up-seaweedfs) for how to set up a simple SeaweedFS cluster.

## Cluster overview

The CLP package is composed of several components (services in orchestrator terminology) including
infrastructure services, schedulers, workers, and supporting services. For a detailed overview of
all services and their dependencies, see the [deployment orchestration design
doc][design-orchestration].

In a multi-host cluster:

* *infrastructure services* and *schedulers* should be run once per cluster (they're singleton
  services).
* *workers* can be run on multiple hosts to increase parallelism.

## Configuring CLP

To configure CLP for multi-host deployment, you'll need to:

1. [configure and run CLP's environment setup scripts](#clp-environment-setup).
2. [update CLP's *generated* configuration to support a multi-host deployment](
   #updating-clps-generated-configuration).
3. [distribute and configure the CLP package on all hosts in your cluster](
   #distributing-the-set-up-package).

### CLP environment setup

1. Extract the CLP package on one host (the "setup host").

2. Configure credentials:

   * Copy `etc/credentials.template.yaml` to `etc/credentials.yaml`.
   * Edit `etc/credentials.yaml` to set usernames and passwords.

3. Edit CLP's configuration file:

   * Open `etc/clp-config.yaml`.
   * For each service, set the `host` and `port` fields to the actual hostname/IP and port where you
     plan to run the specific service.
   * When using local filesystem storage (i.e., not S3), set `logs_input.storage.directory`,
     `archive_output.storage.directory`, and `stream_output.storage.directory` to directories on the
     shared filesystem.

4. Set up the CLP package's environment:

   ```bash
   sbin/start-clp.sh --setup-only
   ```

   This will:

   * Validate your configuration
   * Create any necessary directories
   * Generate an `.env` file with all necessary environment variables
   * Create `var/log/.clp-config.yaml` (the container-specific configuration file)
   * Create `var/www/webui/server/dist/settings.json` (the `webui` server's configuration file)

### Updating CLP's generated configuration

The last step in the previous section (`sbin/start-clp.sh --setup-only`) will generate any necessary
configuration files, but they're unsuitable for use across multiple hosts (they're designed for use
on a single host).

:::{note}
As mentioned at the beginning of this guide, this setup will be made simpler in a future release.
:::

To update the generated configuration files for use across multiple hosts:

1. Edit `var/log/.clp-config.yaml`:

    * Update all `host` fields to use the actual hostname or IP address where each service will run
      (matching what you configured in `etc/clp-config.yaml`).
    * Similarly, update any `port` fields.
    * For example, if your database runs on `192.168.1.10:3306`, ensure `database.host` is set to
      `192.168.1.10` and `database.port` is `3306`.

2. Edit `var/www/webui/server/dist/settings.json`:

    * Update `SqlDbHost` to the actual hostname or IP address of your database service.
    * Update `SqlDbPort` if you changed the database port.
    * Update `MongoDbHost` to the actual hostname or IP address of your results cache service.
    * Update `MongoDbPort` if you changed the results cache port.

### Distributing the set-up package

With the package set up, we can now distribute it to all hosts in the cluster:

1. Copy the set-up package to all hosts where you want to run CLP services.

    * Ensure the package is copied to the same location on every host or else, on each host, you'll
      need to modify the paths in `.env` as appropriate.

2. Configure worker concurrency (optional):

   On each worker host, edit the `.env` file to adjust worker concurrency settings as needed:

   * `CLP_COMPRESSION_WORKER_CONCURRENCY`
   * `CLP_QUERY_WORKER_CONCURRENCY`
   * `CLP_REDUCER_CONCURRENCY`

   Recommended settings:

   * If workers are started on separate hosts, set each concurrency value to match the CPU count on
     that host.
   * If compression and query/reducer workers are started on the same host, set each concurrency
     value to half the CPU count (e.g., for a 16-core host, set all three to 8).

## Starting CLP

You can start CLP across multiple hosts by starting each service on the relevant host. The commands
below indicate how to do so, with comments indicating the startup order and dependencies between
services.

:::{note}
For **clp-json + Presto** deployments (`package.storage_engine`: `clp-s` with
`package.query_engine`: `presto`), you can omit starting the `query-scheduler`, `query-worker`, and
`reducer` services.
:::

:::{tip}
If you want to use your own MariaDB/MySQL or MongoDB servers instead of the Docker Compose managed
databases, see the [external database setup guide](guides-external-database.md). When using external
databases, skip starting the `database` and `results-cache` services below.
:::

All commands below assume you are running them from the root of the CLP package directory.

```bash
################################################################################
# Infrastructure services
################################################################################

# Start database (skip if using external database)
docker compose \
  --project-name "clp-package-$(cat var/log/instance-id)" \
  up database \
    --no-deps --wait

# Initialize database
docker compose \
  --project-name "clp-package-$(cat var/log/instance-id)" \
  up db-table-creator \
    --no-deps

# Start queue (if using Celery)
docker compose \
  --project-name "clp-package-$(cat var/log/instance-id)" \
  up queue \
    --no-deps --wait

# Start redis (if using Celery)
docker compose \
  --project-name "clp-package-$(cat var/log/instance-id)" \
  up redis \
    --no-deps --wait

# Start results cache (skip if using external MongoDB)
docker compose \
  --project-name "clp-package-$(cat var/log/instance-id)" \
  up results-cache \
    --no-deps --wait

# Initialize results cache
docker compose \
  --project-name "clp-package-$(cat var/log/instance-id)" \
  up results-cache-indices-creator \
    --no-deps

################################################################################
# Controller services (schedulers, UI, and supporting services)
################################################################################

# Start compression scheduler
docker compose \
  --project-name "clp-package-$(cat var/log/instance-id)" \
  up compression-scheduler \
    --no-deps --wait
    
# Start Spider scheduler (optional, only if using Spider)
docker compose \
    --project-name "clp-package-$(cat var/log/instance-id)" \
    up spider-scheduler \
      --no-deps --wait

# Start query scheduler
docker compose \
  --project-name "clp-package-$(cat var/log/instance-id)" \
  up query-scheduler \
    --no-deps --wait

# Start API server
docker compose \
  --project-name "clp-package-$(cat var/log/instance-id)" \
  up api-server \
    --no-deps --wait

# Start log-ingestor (optional, only if configured)
docker compose \
  --project-name "clp-package-$(cat var/log/instance-id)" \
  up log-ingestor \
    --no-deps --wait

# Start webui
docker compose \
  --project-name "clp-package-$(cat var/log/instance-id)" \
  up webui \
    --no-deps --wait

# Start garbage collector (optional, only if retention is configured)
docker compose \
  --project-name "clp-package-$(cat var/log/instance-id)" \
  up garbage-collector \
    --no-deps --wait

# Start MCP server (optional, only if configured)
docker compose \
  --project-name "clp-package-$(cat var/log/instance-id)" \
  up mcp-server \
    --no-deps --wait

################################################################################
# Worker services (can be started on multiple hosts)
################################################################################

# Start compression worker (if using Celery)
docker compose \
  --project-name "clp-package-$(cat var/log/instance-id)" \
  up compression-worker \
    --no-deps --wait
    
# Start Spider compression worker (optional, only if using Spider)
docker compose \
  --project-name "clp-package-$(cat var/log/instance-id)" \
  up spider-compression-worker \
    --no-deps --wait

# Start query worker
docker compose \
  --project-name "clp-package-$(cat var/log/instance-id)" \
  up query-worker \
    --no-deps --wait

# Start reducer
docker compose \
  --project-name "clp-package-$(cat var/log/instance-id)" \
  up reducer \
    --no-deps --wait
```

:::{note}
To increase parallelism, start worker services (`compression-worker`, `query-worker`, `reducer`) on
multiple hosts.
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

To stop CLP, on every host where it's running, run:

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

## Operational logging

CLP uses [Fluent Bit][fluent-bit] to collect and aggregate operational logs from all services. Logs
are written to `var/log/<component>/` on each host.

### Current limitations in multi-host deployments

:::{warning}
In multi-host deployments, operational logs are currently stored locally on each host. This means:

* Each host runs its own Fluent Bit instance that collects logs from containers on that host.
* Logs are written to the local filesystem (`var/log/<component>/`) and are not automatically
  aggregated across hosts.
* To view logs from a specific host, you must access that host directly.
:::

### Workarounds

Until centralized log aggregation is implemented, you can:

1. **Use a shared filesystem**: Mount `var/log/` to a shared filesystem (e.g., NFS, SeaweedFS) so
   all hosts write logs to the same location. Note that you may need to include the hostname in the
   log path to avoid conflicts.

2. **Use `docker compose logs`**: View real-time logs for a service using:

   ```bash
   docker compose --project-name clp-package-<instance-id> logs -f <service-name>
   ```

3. **Manual log collection**: Periodically copy logs from each host to a central location for
   analysis.

### Future improvements

A future release will add support for shipping logs to S3 or other centralized storage, enabling:

* Centralized log aggregation across all hosts
* Log viewing through the webui regardless of which host generated the logs
* Long-term log retention with tiered storage (hot/warm/cold)

[fluent-bit]: https://fluentbit.io/

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
