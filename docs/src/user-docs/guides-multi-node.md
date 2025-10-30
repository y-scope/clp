# Multi-node deployment

A multi-node deployment allows you to run CLP across a distributed set of hosts.

:::{note}
Multi-node deployment requires manual orchestration of Docker Compose services across your
cluster. This guide explains how to use the `--setup-only` flag and Docker Compose to deploy
CLP components across multiple hosts.
:::

## Requirements

* [Docker] and [Docker Compose][docker-compose]
  * If you're not running as root, ensure Docker can be run
    [without superuser privileges][docker-non-root].
* One or more hosts networked together
* A shared filesystem accessible by all worker hosts (e.g., NFS, [SeaweedFS])
  * See [below](#setting-up-seaweedfs) for how to set up a simple SeaweedFS cluster.

## Cluster overview

The CLP package is composed of several services that can be categorized as infrastructure services,
schedulers, workers, and supporting services. For a detailed overview of all services and their
dependencies, see the [deployment orchestration design doc][design-orchestration].

In a multi-node cluster:
* **Infrastructure services** and **schedulers** should be run once per cluster (singleton services)
* **Workers** can be run on multiple hosts to increase parallelism

:::{note}
Running additional worker instances on different hosts increases the parallelism of compression and
search/aggregation jobs.
:::

## Configuring CLP

1. **Extract the CLP package** on one host (the "setup host") where you will prepare the
   configuration.

2. **Configure credentials**:
   1. Copy `etc/credentials.template.yml` to `etc/credentials.yml`.
   2. Edit `etc/credentials.yml`:
      1. Uncomment the file.
      2. Choose appropriate usernames and passwords for database, queue, and Redis.
         * Note that these are *new* credentials that will be used by the components.

3. **Configure CLP settings**:
   1. Edit `etc/clp-config.yml`:
      1. Uncomment the file.
      2. Set the `host` config for each controller component to the hostname/IP where you plan to
         run them:
         * `database.host` - Host for the database service
         * `queue.host` - Host for the message queue service
         * `redis.host` - Host for the Redis service
         * `results_cache.host` - Host for the results cache (MongoDB) service
         * `query_scheduler.host` - Host for the query scheduler service
         * `webui.host` - Host for the Web UI service
         * You can run all controller components on a single host or distribute them across
           multiple hosts.
      3. Change any controller component ports that will conflict with services you already have
         running.
      4. Set `archive_output.storage.directory` to a directory on the shared filesystem.
         * Ideally, the directory should be empty or should not yet exist (CLP will create it)
           since CLP will write several files and directories directly to the given directory.
      5. If you want to enable the MCP server, configure `mcp_server` settings; otherwise, leave
         it as `null`.

4. **Initialize the package environment**:

   Run the following command on the setup host:

   ```bash
   sbin/start-clp.sh --setup-only
   ```

   This will:
   * Validate your configuration
   * Create necessary directories
   * Generate the `.env` file with all environment variables
   * Create `var/log/.clp-config.yml` (the container-specific config)

5. **Customize the generated configuration for multi-node deployment**:

   After running `--setup-only`, the generated `var/log/.clp-config.yml` file will have hostnames
   converted to Docker Compose service names (e.g., `database`, `queue`) and ports reset to
   defaults. For multi-node deployment, you **must** update this file to use actual network-reachable
   hostnames/IPs:

   1. **Edit `var/log/.clp-config.yml`**:
      * Update all `host` fields to use the actual hostname or IP address where each service will
        run (matching what you configured in `etc/clp-config.yml`).
      * Verify that ports match your configuration.
      * For example, if your database runs on `192.168.1.10:3306`, ensure `database.host` is set to
        `192.168.1.10` and `database.port` is `3306`.

   2. **Edit `.env` file** (if needed):
      * Update any host-specific paths or settings.

   :::{important}
   The transformation to service names is intended for single-node Docker Compose deployments.
   For multi-node deployments, services on different hosts need to communicate via actual network
   addresses, not Docker service names.
   :::

6. **Determine the instance ID**:

   The instance ID is automatically generated and stored in `var/log/instance-id`. You'll need
   this for the Docker Compose project name. Read it with:

   ```bash
   cat var/log/instance-id
   ```

7. **Distribute the package** to all hosts where you want to run CLP components:

   Copy the entire CLP package directory (including the generated `.env`, `var/log/.clp-config.yml`,
   and `var/log/instance-id` files) to all hosts in your cluster.

## Starting CLP

Before starting each CLP component, note that some components must be started before others due to
dependencies. We organize the components into startup groups below, where components in a group can
be started in any order, but all components in a group must be started (and healthy/completed)
before starting components in the next group.

**Startup Group 1** (infrastructure databases):

* `database`
* `queue`
* `redis`
* `results-cache`

**Startup Group 2** (one-time initialization jobs):

* `db-table-creator` (run after `database` is healthy)
* `results-cache-indices-creator` (run after `results-cache` is healthy)

:::{note}
These initialization jobs run once and exit. Wait for them to complete successfully before
proceeding to the next group.
:::

**Startup Group 3** (schedulers and UI):

* `compression-scheduler`
* `query-scheduler`
* `webui`
* `garbage-collector` (optional, only if retention is configured)
* `mcp-server` (optional, only if MCP server is configured)

**Startup Group 4** (workers):

* `compression-worker`
* `query-worker`
* `reducer`

### Starting a service

For each component, on the host where you want to run it, use the following command format:

```bash
docker compose --project-name clp-package-<instance-id> \
  --file docker-compose.yaml \
  up <service-name> --no-deps --detach --wait
```

Replace:
* `<instance-id>` with the instance ID from `var/log/instance-id`
* `<service-name>` with one of the service names from the startup groups above

**Example**: To start the database service with instance ID `a1b2`:

```bash
docker compose --project-name clp-package-a1b2 \
  --file docker-compose.yaml \
  up database --no-deps --detach --wait
```

:::{tip}
The `--no-deps` flag tells Docker Compose to start only the specified service without starting its
dependencies. This is important for multi-node deployments where dependencies may be running on
other hosts. The `--wait` flag ensures the command waits for the service to be healthy before
returning.
:::

:::{important}
For initialization jobs (`db-table-creator` and `results-cache-indices-creator`), omit the
`--detach` flag to run them in the foreground and wait for completion:

```bash
docker compose --project-name clp-package-a1b2 \
  --file docker-compose.yaml \
  up db-table-creator --no-deps

docker compose --project-name clp-package-a1b2 \
  --file docker-compose.yaml \
  up results-cache-indices-creator --no-deps
```
:::

:::{note}
For **clp-text** deployments (storage engine: `clp`), use `docker-compose.yaml`. For **clp-json**
base deployments (storage engine: `clp-s` without query engine), use `docker-compose-base.yaml`
instead.
:::

### Starting workers on multiple hosts

You can start worker components (`compression-worker`, `query-worker`, `reducer`) on multiple hosts
to increase parallelism:

1. Copy the package to each worker host
2. Ensure the shared filesystem is mounted at the same path on all worker hosts
3. Run the appropriate `docker compose up` command on each worker host

**Example**: Starting a compression worker on three different hosts:

```bash
# On worker-host-1
docker compose --project-name clp-package-a1b2 \
  --file docker-compose.yaml \
  up compression-worker --no-deps --detach

# On worker-host-2
docker compose --project-name clp-package-a1b2 \
  --file docker-compose.yaml \
  up compression-worker --no-deps --detach

# On worker-host-3
docker compose --project-name clp-package-a1b2 \
  --file docker-compose.yaml \
  up compression-worker --no-deps --detach
```

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

To stop a service on a host, run:

```bash
docker compose --project-name clp-package-<instance-id> down <service-name>
```

To stop all services on a host, run:

```bash
docker compose --project-name clp-package-<instance-id> down
```

**Example**:

```bash
docker compose --project-name clp-package-a1b2 down compression-worker
```

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
