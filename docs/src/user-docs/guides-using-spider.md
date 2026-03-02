# Using Spider with CLP

[Spider] is a fast and scalable distributed task execution engine that can be used to run tasks.
This guide describes how to set up and use Spider with CLP.

:::{note}
Spider is under active development, and its integration with CLP may change in the future.
Right now, Spider only supports executing CLP compression tasks. Support for search tasks will be added
later.
:::

## Requirements

* [CLP][clp-releases] v0.8.0 or higher
* [Docker] v28 or higher
* [Docker Compose][docker-compose] v2.20.2 or higher

## Set up

To use Spider for CLP compression tasks, you need to [set up CLP](#setting-up-clp-with-spider) with
Spider in configuration.

### Setting up CLP with Spider

1. Follow the [quick-start](quick-start/index.md) guide to download and extract the CLP package,
   but don't start the package just yet.
2. Before starting the package, update the package's config file (`etc/clp-config.yaml`) as follows:

    * Change the `compression_scheduler.type` field to `"spider"`.

      ```yaml
      compression_scheduler:
        type: "spider"
      ```

    * (Optional) Override `database.names.spider` to avoid name conflicts when using self-provisioned database instances.

        ```yaml
        database:
            names:
                spider: "spider-db"
        ```

    * (Optional) Override the `spider_scheduler` default config to change listening host or avoid port conflicts.

        ```yaml
        spider_scheduler:
            host: "localhost"
            port: 6000
        ```

3. (Optional) If you do not intend to use generated credentials, set your own Spider credentials in
   `etc/credentials.yaml` before starting the package.

    ```yaml
    database:
      spider_username: "spider-user"
      spider_password: "spider-pass"
    ```

4. Continue following the [quick-start](./quick-start/index.md#using-clp) guide to start CLP.

[clp-releases]: https://github.com/y-scope/clp/releases
[docker-compose]: https://docs.docker.com/compose/install/
[Docker]: https://docs.docker.com/engine/install/
[Spider]: https://github.com/y-scope/spider
