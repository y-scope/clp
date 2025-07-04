# Multi-node deployment

A multi-node deployment allows you to run CLP across a distributed set of hosts.

## Requirements

* [Docker]
  * If you're not running as root, ensure docker can be run
    [without superuser privileges][docker-non-root].
* Python 3.8 or higher
* One or more hosts networked together
* A distributed filesystem (e.g. [SeaweedFS]) accessible by all worker hosts through a filesystem
  mount
  * See [below](#setting-up-seaweedfs) for how to set up a simple SeaweedFS cluster.

## Cluster overview

The CLP package is composed of several components--controller components and worker components. In a
cluster, there should be a single instance of each controller component and one or more instances of
worker components. The tables below list the components and their functions.

:::{table} Controller components
:align: left

| Component             | Description                                                     |
|-----------------------|-----------------------------------------------------------------|
| database              | Database for archive metadata, compression jobs, and query jobs |
| queue                 | Task queue for schedulers                                       |
| redis                 | Task result storage for workers                                 |
| compression_scheduler | Scheduler for compression jobs                                  |
| query_scheduler       | Scheduler for search/aggregation jobs                           |
| results_cache         | Storage for the workers to return search results to the UI      |
| webui                 | Web server for the UI                                           |
:::

:::{table} Worker components
:align: left

| Component          | Description                                                  |
|--------------------|--------------------------------------------------------------|
| compression_worker | Worker processes for compression jobs                        |
| query_worker       | Worker processes for search/aggregation jobs                 |
| reducer            | Reducers for performing the final stages of aggregation jobs |
:::

:::{note}
Running additional workers increases the parallelism of compression and search/aggregation jobs.
:::

## Configuring CLP

1. Copy `etc/credentials.template.yml` to `etc/credentials.yml`.
2. Edit `etc/credentials.yml`:

    {style=lower-alpha}
    1. Uncomment the file.
    2. Choose an appropriate username and password.
       * Note that these are *new* credentials that will be used by the components.

3. Choose which hosts you would like to use for the controller components.
   * You can use a single host for all controller components.
4. Edit `etc/clp-config.yml`:

    {style=lower-alpha}
    1. Uncomment the file.
    2. Set the `host` config of each controller component to the host that you'd like to run them
       on.
       * If desired, you can run different controller components on different hosts.
    3. Change any of the controller components' ports that will conflict with services you already
       have running.
    4. Set `archive_output.directory` to a directory on the distributed filesystem.
       * Ideally, the directory should be empty or should not yet exist (CLP will create it) since
         CLP will write several files and directories directly to the given directory.

5. Download and extract the package on all nodes.
6. Copy the `credentials.yml` and `clp-config.yml` files that you created above and paste them
   into `etc` on all the hosts where you extracted the package.

## Starting CLP

Before starting each CLP component, note that some components must be started before others. We
organize the components into groups below, where components in a group can be started in any order,
but all components in a group must be started before starting a component in the next group.

**Group 1 components:**

* `database`
* `queue`
* `redis`
* `results_cache`

**Group 2 components:**

* `compression_scheduler`
* `query_scheduler`

**Group 3 components:**

* `compression_worker`
* `query_worker`
* `reducer`

For each component, on the host where you want to run the component, run:

```bash
sbin/start-clp.sh <component>
```

Where `<component>` is the name of the component in the groups above.

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

If you need to stop the cluster, run:

```bash
sbin/stop-clp.sh
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

     ```
     weed mount -filer "<master-host>:8888" -dir <mount-path>
     ```

     * `<master-host>` is the hostname/IP of the master host.
     * `<mount-path>` is the path where you want the mount to be.

[Docker]: https://docs.docker.com/engine/install/
[docker-non-root]: https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user
[SeaweedFS]: https://github.com/seaweedfs/seaweedfs
[seaweedfs-docs]: https://github.com/seaweedfs/seaweedfs/blob/master/README.md
[seaweedfs-install-docs]: https://github.com/seaweedfs/seaweedfs?tab=readme-ov-file#quick-start-with-single-binary
