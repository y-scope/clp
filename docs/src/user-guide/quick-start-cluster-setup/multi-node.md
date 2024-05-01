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

| Component             | Description                                                      |
|-----------------------|------------------------------------------------------------------|
| database              | Database for archive metadata, compression jobs, and search jobs |
| queue                 | Task queue for schedulers                                        |
| redis                 | Task result storage for workers                                  |
| compression_scheduler | Scheduler for compression jobs                                   |
| search_scheduler      | Scheduler for search jobs                                        |
| results_cache         | Storage for the workers to return search results to the UI       |
| webui                 | Web server for the UI                                            |
:::

:::{table} Worker components
:align: left

| Component          | Description                                                  |
|--------------------|--------------------------------------------------------------|
| compression_worker | Worker processes for compression tasks                       |
| search_worker      | Worker processes for search/aggregation tasks                |
| reducer            | Reducers for performing the final stages of aggregation jobs |
:::

:::{note}
Running additional workers increases the parallelism of compression and search jobs.
:::

## Configuring CLP

1. Copy `etc/credentials.template.yml` to `etc/credentials.yml`.
2. Edit `etc/credentials.yml`:

    {style=lower-alpha}
    1. Uncomment the file.
    2. Set the username and password to something appropriate.
       * Note that these are *new* credentials that will be used by the components.

3. Choose which hosts you would like to use for the controller components.
   * You can use a single host for all controller components.
4. Edit `etc/clp-config.yml`:

    {style=lower-alpha}
    1. Uncomment the file.
    2. Set the `host` config of each controller component to the hosts you choose in step 3.
    3. Change any of the controller components' ports that will conflict with services you already
       have running.
    4. Set `archive_output.directory` to a directory on the distributed filesystem.

5. Download and extract the package on all nodes.
6. Take the versions of `credentials.yml` and `clp-config.yml` that you created above and copy them
   into `etc` on all the hosts where you extracted the package.

## Starting CLP

For each component, on the host where you want to run the component, run:

```bash
sbin/start-clp.sh <component>
```

## Using CLP

Check out the [compression](../quick-start-compression/index) and
[search](../quick-start-search/index) guides to compress and search your logs.

## Stopping CLP

If you need to stop the cluster, run:

```bash
sbin/stop-clp.sh
```

(setting-up-seaweedfs)=
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
