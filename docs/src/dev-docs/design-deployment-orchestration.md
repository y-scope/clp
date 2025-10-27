# Deployment orchestration

The CLP package is composed of several components that are currently designed to be deployed in a
set of containers that are orchestrated using a framework like [Docker Compose][docker-compose].
This document explains the architecture of that orchestration and any associated nuances.

## Architecture

[Figure 1](#figure-1) shows the components (*services* in orchestrator terminology) in the CLP
package as well as their dependencies. The CLP package consists of several long-running services
(e.g., `database`) and some one-time initialization jobs (e.g., `db-table-creator`). Some of the
long-running services depend on the successful completion of the one-time jobs (e.g., `webui`
depends on `results-cache-indices-creator`), while others depend on the health of other long-running
services (e.g., `compression-scheduler` depends on `queue`).

[Table 1](#table-1) below lists the services their functions, while [Table 2](#table-2) lists the
one-time initialization jobs and their functions.

(figure-1)=
::::{card}

:::{mermaid}
%%{
    init: {
        "theme": "base",
        "themeVariables": {
            "primaryColor": "#0066cc",
            "primaryTextColor": "#fff",
            "primaryBorderColor": "transparent",
            "lineColor": "#007fff",
            "secondaryColor": "#007fff",
            "tertiaryColor": "#fff"
        }
    }
}%%
graph LR
  %% Services
  database["database (MySQL)"]
  queue["queue (RabbitMQ)"]
  redis["redis (Redis)"]
  results_cache["results-cache (MongoDB)"]
  compression_scheduler["compression-scheduler"]
  query_scheduler["query-scheduler"]
  compression_worker["compression-worker"]
  query_worker["query-worker"]
  reducer["reducer"]
  webui["webui"]
  mcp_server["mcp-server"]

  garbage_collector["garbage-collector"]

  %% One-time jobs
  db_table_creator["db-table-creator"]
  results_cache_indices_creator["results-cache-indices-creator"]

  %% Dependencies
  database -->|healthy| db_table_creator
  results_cache -->|healthy| results_cache_indices_creator
  db_table_creator -->|completed_successfully| compression_scheduler
  queue -->|healthy| compression_scheduler
  redis -->|healthy| compression_scheduler
  db_table_creator -->|completed_successfully| query_scheduler
  queue -->|healthy| query_scheduler
  redis -->|healthy| query_scheduler
  query_scheduler -->|healthy| reducer
  results_cache_indices_creator -->|completed_successfully| reducer
  db_table_creator -->|completed_successfully| webui
  results_cache_indices_creator -->|completed_successfully| webui
  db_table_creator -->|completed_successfully| mcp_server
  results_cache_indices_creator -->|completed_successfully| mcp_server
  db_table_creator -->|completed_successfully| garbage_collector
  results_cache_indices_creator -->|completed_successfully| garbage_collector

  subgraph Databases
    database
    queue
    redis
    results_cache
  end

  subgraph Initialization jobs
    db_table_creator
    results_cache_indices_creator
  end

  subgraph Schedulers
    compression_scheduler
    query_scheduler
  end

  subgraph Workers
    compression_worker
    query_worker
    reducer
  end

  subgraph UI & Management
    webui
    garbage_collector
  end

  subgraph AI
    mcp_server
  end


+++
**Figure 1**: Orchestration architecture of the services in the CLP package.
::::

(table-1)=
::::{card}

:::{table}
:align: left

| Service               | Description                                                     |
|-----------------------|-----------------------------------------------------------------|
| database              | Database for archive metadata, compression jobs, and query jobs |
| queue                 | Task queue for schedulers                                       |
| redis                 | Task result storage for workers                                 |
| compression_scheduler | Scheduler for compression jobs                                  |
| query_scheduler       | Scheduler for search/aggregation jobs                           |
| results_cache         | Storage for the workers to return search results to the UI      |
| compression_worker    | Worker processes for compression jobs                           |
| query_worker          | Worker processes for search/aggregation jobs                    |
| reducer               | Reducers for performing the final stages of aggregation jobs    |
| webui                 | Web server for the UI                                           |
| mcp_server            | MCP server for AI agent to access CLP functionalities           |
| garbage_collector     | Process to manage data retention                                |

:::

+++
**Table 1**: Long-running services in the CLP package.
::::

(table-2)=
::::{card}

:::{table}
:align: left

| Job                           | Description                                           |
|-------------------------------|-------------------------------------------------------|
| db-table-creator              | Creates and initializes database tables               |
| results-cache-indices-creator | Creates a single-node replica set and sets up indices |

:::

+++
**Table 2**: One-time initialization jobs in the CLP package.
::::

## Code structure

The orchestration code is split up into:

* `BaseController` that defines:
  * common logic for preparing the environment variables, configuration files, and directories
    necessary for each service.
  * abstract methods that orchestrator-specific derived classes must implement in order to
    orchestrate a deployment.
* `<Orchestrator>Controller` that implements (and/or overrides) any of the methods in
  `BaseController` (`<Orchestrator>` is a placeholder for the specific orchestrator for which the
  class is being implemented).

## Docker Compose orchestration

This section explains how we use Docker Compose to orchestrate the CLP package and is broken into
the following subsections:

* [Setting up the Docker Compose project's environment](#setting-up-the-environment)
* [Starting and stoping the Docker Compose project](#starting-and-stopping-the-project)
* [Deployment types](#deployment-types)
* [Implementation details](#implementation-details)
* [Troubleshooting](#troubleshooting)

### Setting up the environment

Several services require configuration values to be passed in through the CLP package's config file,
environment variables, and/or command line arguments. Since the services are running in containers,
some of these configuration values need to be modified for the orchestration environment.
Specifically:

1. Paths on the host must be converted to appropriate paths in the container.
2. Component hostnames must be converted to service names, and component ports must be converted to the component's default ports.
    * This ensures that in the Docker Compose configuration, services can communicate over fixed, predictable hostnames and ports rather than relying on configurable variables.

To achieve this, before starting the deployment, `DockerComposeController.start` generates:

* a CLP configuration file (`<clp-package>/var/log/.clp-config.yml` on the host) specific to the
  Docker Compose project environment.
* an environment variable file (`<clp-package>/.env`) for any other configuration values.
* any necessary directories (e.g., data output directories).

The Docker Compose project then passes those environment variables to the relevant services, either
as environment variables or command line arguments, as necessary.

### Starting and stopping the project

To start and stop the project, `DockerComposeController` simply invokes `docker compose up` or
`docker compose down` as appropriate. However, to allow multiple CLP packages to be run on the same
host, we explicitly specify a project name for the project, where the name is based on the package's
instance ID.

### Deployment Types

CLP supports two deployment types determined by the `package.query_engine` configuration setting.

1. **BASE**: For deployments using [Presto][presto-integration] as the query engine. This deployment
   only uses `docker-compose.base.yaml`.
2. **FULL**: For deployments using one of CLP's native query engines. This uses both
   `docker-compose.base.yaml` and `docker-compose.yaml`.

### Implementation details

One notable implementation detail is in how we handle mounts that are only necessary under certain
configurations. For instance, the input logs mount is only necessary when the `logs_input.type` is
`fs`. If `logs_input.type` is `s3`, we shouldn't mount some random directory from the user's
host filesystem into the container. However, Docker doesn't provide a mechanism to perform
conditional mounts. Instead, we use Docker's variable interpolation to conditionally mount an empty
tmpfs mount into the container. This strategy is used wherever we need a conditional mount.

### Troubleshooting

If you encounter issues with the Docker Compose deployment, first determine the instance ID for your
deployment by checking the content of `<clp-package>/var/log/instance-id`. Then run one of the
commands below as necessary.

1. Check service status:

   ```bash
   docker compose --project-name clp-package-<instance-id> ps
   ```

2. View service logs:

   ```bash
   docker compose --project-name clp-package-<instance-id> logs <service-name>
   ```

3. Validate configuration:

   ```bash
   docker compose config
   ```

[docker-compose]: https://docs.docker.com/compose/
[presto-integration]: ../user-docs/guides-using-presto.md
