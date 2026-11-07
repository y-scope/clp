# Docker Compose deployment

The supplied Docker Compose configuration runs a Spider cluster on a single host. It includes the
compression and query coordinators, Spider storage and scheduler, and worker pools for compression
and search. For a multi-host deployment, use the [Kubernetes guide][kubernetes-guide].

## Requirements

* [Docker] and [Docker Compose][docker-compose]
  * If you're not running as root, ensure Docker can be run [without superuser
    privileges][docker-non-root].
* Filesystem input and archive directories accessible on the deployment host
  * See [below](#setting-up-seaweedfs) for a SeaweedFS filesystem setup.

## Configuring CLP

1. Extract the CLP package.
2. Copy `etc/credentials.template.yaml` to `etc/credentials.yaml` and set the credentials.
3. Edit `etc/clp-config.yaml`:
   * Set `logs_input.directory` to the directory containing the logs to compress.
   * Set `archive_output.storage.directory` and `stream_output.storage.directory` when using
     filesystem storage.
   * Adjust `spider.worker.pools.general.replicas`, `spider.worker.pools.compression.replicas`, and
     `spider.worker.pools.query.replicas` to allocate workers for the host's available resources.

The compression coordinator and workers mount the same input directory read-only. Workers also
mount the configured archive and staging directories.

To use external MariaDB/MySQL or MongoDB instances, follow the
[external database setup guide](guides-external-database.md) and remove the corresponding
`database` or `results_cache` entry from `bundled`.

## Starting CLP

Run from the root of the CLP package:

```bash
sbin/start-clp.sh
```

The startup script validates configuration, creates directories, generates `.env` and
`var/log/.clp-config.yaml`, initializes the databases, and starts the configured services.
Use `sbin/start-clp.sh --setup-only` to generate the environment without starting services.

The Compose services include:

* `database` and `results-cache` for CLP metadata and search results
* `spider-database`, `spider-storage`, and `spider-scheduler` for Spider task execution
* `compression-coordinator` and `query-coordinator` for job submission and monitoring
* `spider-worker`, `spider-worker-compression`, and `spider-worker-query` for the worker pools
* `api-server` and `webui` for the user interface

The bundled Spider scheduler and workers use container-network endpoints on the same host.
Copying this Compose configuration to other hosts does not configure a distributed cluster.

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

## Monitoring and debugging

First, determine your instance ID from `<clp-package>/var/log/instance-id`.

To check the status of services:

```bash
docker compose --project-name clp-package-<instance-id> ps
```

To view logs for a specific service:

```bash
docker compose --project-name clp-package-<instance-id> logs -f <service-name>
```

To execute commands in a running container:

```bash
docker compose --project-name clp-package-<instance-id> exec <service-name> /bin/bash
```

To validate your Docker Compose configuration:

```bash
docker compose config
```

---

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

[Docker]: https://docs.docker.com/engine/install/
[docker-compose]: https://docs.docker.com/compose/
[docker-non-root]: https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user
[kubernetes-guide]: guides-k8s-deployment.md
[seaweedfs-docs]: https://github.com/seaweedfs/seaweedfs/blob/master/README.md
[seaweedfs-install-docs]: https://github.com/seaweedfs/seaweedfs?tab=readme-ov-file#quick-start-with-single-binary
