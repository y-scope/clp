# Single-node deployment

A single-node deployment allows you to run CLP on a single host.

## Requirements

* [Docker]
  * If you're not running as root, ensure `docker` can be run
    [without superuser privileges][docker-non-root].
  * If you're using Docker Desktop, ensure version 4.34 or higher is installed, and
    [host networking is enabled][docker-desktop-host-networking].
* Python 3.8 or higher

## Starting CLP

```bash
sbin/start-clp.sh
```

:::{note}
If CLP fails to start (e.g., due to a port conflict), try adjusting the settings in
`etc/clp-config.yml` and then run the start command again.
:::

## Using CLP

Check out the [compression](../quick-start-compression/index) and
[search](../quick-start-search/index) guides to compress and search your logs.

## Stopping CLP

If you need to stop the cluster, run:

```bash
sbin/stop-clp.sh
```

[Docker]: https://docs.docker.com/engine/install/
[docker-desktop-host-networking]: https://docs.docker.com/engine/network/drivers/host/#docker-desktop
[docker-non-root]: https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user
