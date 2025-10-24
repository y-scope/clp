# Multi-node deployment

A multi-node deployment allows you to run CLP across a distributed set of hosts. The packaged
`start-clp.sh` wrapper (which calls `start_clp.py`) now exposes a `--target` flag so each host can
launch only the services it should run, while sharing the same Compose project.

## Roles and targets

| Target               | Responsibilities | Notes |
|----------------------|------------------|-------|
| `controller`         | Databases, schedulers, garbage collector | Run this first so dependencies are available. |
| `ui`                 | Web UI           | Starts only the `webui` service with no local dependencies. |
| `compression-worker` | Compression workers | Accepts `--num-workers` to override process count. |
| `query-worker`       | Query workers    | Available only when CLP's legacy query engine is enabled. |
| `reducer`            | Reducer processes | Available only when CLP's legacy query engine is enabled. |
| `mcp`                | MCP server       | Optional; requires an `mcp_server` block in `clp-config.yml`. |

When no target is specified, `start-clp.sh` launches the full stack on a single machine (the
previous behaviour). The controller inspects `package.query_engine` and sets
`CLP_ENABLE_LEGACY_SEARCH=0` automatically when Presto is selected, preventing the query scheduler,
query workers, and reducer from starting on any host.

## Preparation

1. Install the CLP package on each host that will participate in the deployment.
2. Copy the same `clp-config.yml` (and any credentials files) to every host. Ensure network
   addresses in the config are reachable from all nodes.
3. If you plan to store data or logs on shared storage, mount those paths before launching CLP.
4. Confirm Docker Engine and the Compose plugin meet the versions listed in the installation guide.

## Launch procedure

1. On the controller host, run:

   ```bash
   ./sbin/start-clp.sh --target controller
   ```

   This creates the Compose project, primes configuration, and launches the core services.

2. On additional hosts, start the desired roles. Examples:

   ```bash
   # Web UI host
   ./sbin/start-clp.sh --target ui

   # Dedicated compression workers with four processes
   ./sbin/start-clp.sh --target compression-worker --num-workers 4

   # Legacy search workers (only when CLP query engine is in use)
   ./sbin/start-clp.sh --target query-worker --num-workers 8
   ./sbin/start-clp.sh --target reducer --num-workers 2

   # MCP server host
   ./sbin/start-clp.sh --target mcp
   ```

   Each invocation adds services to the existing Compose project without interrupting containers
   launched on other nodes.

## Stopping the deployment

Run `./sbin/stop-clp.sh` from any host that has the package installed. It shuts down the entire
Compose project regardless of which targets were used to start individual services.

## Troubleshooting tips

* Use `docker compose --project-name clp-package-$(cat var/log/instance-id) ps` to verify which
  services are up.
* If a worker fails to connect to core services, confirm that the controller host's addresses in
  `clp-config.yml` are reachable from the worker host, and rerun `start-clp.sh` for that worker after
  updating the configuration.
