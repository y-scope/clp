# Package scripts

The CLP package provides shell scripts in the `sbin/` directory for managing and operating CLP.

* [start-clp.sh](#start-clpsh) for starting CLP services.
* [stop-clp.sh](#stop-clpsh) for stopping CLP services.
* [Admin tools][admin-tools] for managing archives and datasets.

---

## start-clp.sh

Starts all CLP services.

### Usage

```bash
sbin/start-clp.sh [OPTIONS]
```

### Options

| Option                    | Description                                                                                                                     |
|---------------------------|---------------------------------------------------------------------------------------------------------------------------------|
| `-c, --config PATH`       | CLP package configuration file. Defaults to `etc/clp-config.yaml`.                                                              |
| `--restart-policy POLICY` | Docker restart policy for containers. See [Restart policies](#restart-policies) below. Defaults to `on-failure:3`. |
| `--setup-only`            | Validate configuration and prepare directories without starting services. Useful for verifying configuration before deployment. |
| `-v, --verbose`           | Enable debug logging.                                                                                                           |

### Restart policies

The `--restart-policy` option accepts the following values:

| Policy                     | Description                                                            |
|----------------------------|------------------------------------------------------------------------|
| `no`                       | Never restart containers automatically.                                |
| `always`                   | Always restart containers regardless of exit status.                   |
| `unless-stopped`           | Restart containers unless explicitly stopped.                          |
| `on-failure`               | Restart only if the container exits with a non-zero status.            |
| `on-failure:<max-retries>` | Restart on failure up to `<max-retries>` times (e.g., `on-failure:3`). |

:::{note}
Restart policies are primarily meant for handling container failures.

* Use `stop-clp.sh` to properly stop CLP.
* Initialization jobs (`db-table-creator`, `results-cache-indices-creator`) always use
  `on-failure:3` regardless of this setting.
* Docker daemon restart (e.g., system reboot or `systemctl restart docker`) may cause some services
  to restart unexpectedly:
  * If `stop-clp.sh` was run beforehand, services stay stopped since containers are removed, not
    just stopped.
  * With `on-failure` policies, the daemon records container exit codes on shutdown. On restart, it
    restarts containers that exited non-zero. Most CLP services exit cleanly (code 0) and won't
    restart, but some may restart and eventually fail without their dependencies.
  * If services restart unexpectedly after a Docker daemon restart, run `stop-clp.sh` to clean up
    any partially started services bofore running `start-clp.sh`.
:::

---

## stop-clp.sh

Stops all running CLP services.

### Usage

```bash
sbin/stop-clp.sh [OPTIONS]
```

### Options

| Option              | Description                                                        |
|---------------------|--------------------------------------------------------------------|
| `-c, --config PATH` | CLP package configuration file. Defaults to `etc/clp-config.yaml`. |

[admin-tools]: ../reference-admin-tools.md
