# Using Spider with CLP

[Spider] is a fast and scalable distributed task execution engine that can be used to run tasks.
This guide describes how to set up and use Spider with CLP.

:::{note}
Spider is under active development, and its integration with CLP may change in the future.
Right now, Spider only supports executing CLP compression tasks. Support for search tasks will be added
later.
:::

## Requirements
* [CLP][clp-releases] v0.7.0 or higher
* [Docker] v28 or higher
* [Docker Compose][docker-compose] v2.20.2 or higher
* Python
* python3-venv (for the version of Python installed)

## Set up
To use Spider for CLP compression tasks, you need to [set up CLP](#setting-up-clp-with-spider) with
Spider in configuration.

### Setting up CLP with Spider

1. Follow the [quick-start](quick-start/index.md) guide to download and extract the CLP package,
   but don't start the package just yet.
2. Before starting the package, update the package's config file (`etc/clp-config.yaml`) as follows:

    * Set the `compression_scheduler.type` key to `"spider"`.

      ```yaml
      compression_scheduler:
        type: "spider"
      ```
      
    * (Optional) Set the `spider_db`.
   
        ```yaml
        spider_db:
            db_name: "spider-db"
        ```

    * (Optional) Set the `spider_scheduler`.
   
        ```yaml
        spider_scheduler:
            host: "localhost"
            port: 6000
        ```
3. (Optional) Before starting the package, update the package's credential file (`etc/credentials.yaml`)
   to add Spider database credentials as follows:

    ```yaml
    spider_db:
      username: "spider_user"
      password: "spider_password"
    ```
4. Continue following the [quick-start](./quick-start/index.md#using-clp) guide to start CLP.

[clp-releases]: https://github.com/y-scope/clp/releases
[docker-compose]: https://docs.docker.com/compose/install/
[Docker]: https://docs.docker.com/engine/install/
[Spider]: https://github.com/y-scope/spider