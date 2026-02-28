# Deployment orchestration

The CLP package comprises several components that are designed to be deployed in a set of
interdependent containers, and orchestrated by a framework that ensures the containers work together
to facilitate CLP's different functions correctly. This document explains the architecture of the
package components, and describes the two orchestration frameworks that CLP supports:

* [Docker Compose][docker-compose]
* [Kubernetes][kubernetes] (via [Helm][helm])

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
  spider_scheduler["spider-scheduler"]
  compression_worker["compression-worker"]
  spider_compression_worker["spider-compression-worker"]
  query_worker["query-worker"]
  reducer["reducer"]
  api_server["api-server"]
  garbage_collector["garbage-collector"]
  webui["webui"]
  mcp_server["mcp-server"]
  log_ingestor["log-ingestor"]

  %% One-time jobs
  db_table_creator["db-table-creator"]
  results_cache_indices_creator["results-cache-indices-creator"]

  %% Dependencies
  %% Link 0-1: Database --> Database initialization jobs
  database -->|healthy| db_table_creator
  results_cache -->|healthy| results_cache_indices_creator
  linkStyle 0,1 stroke:#ffa500

  %% Link 2-5: Celery dependencies --> Schedulers
  queue -->|healthy| compression_scheduler
  redis -->|healthy| compression_scheduler
  queue -->|healthy| query_scheduler
  redis -->|healthy| query_scheduler
  linkStyle 2,3,4,5 stroke:#ff0000

  %% Link 6: Schedulers --> Workers
  query_scheduler -->|healthy| reducer
  linkStyle 6 stroke:#800080

  %% Link 7-15: Database initialization job --> Services
  db_table_creator -->|completed_successfully| api_server
  db_table_creator -->|completed_successfully| compression_scheduler
  db_table_creator -->|completed_successfully| garbage_collector
  db_table_creator -->|completed_successfully| log_ingestor
  db_table_creator -->|completed_successfully| mcp_server
  db_table_creator -->|completed_successfully| query_scheduler
  db_table_creator -->|completed_successfully| spider_compression_worker
  db_table_creator -->|completed_successfully| spider_scheduler
  db_table_creator -->|completed_successfully| webui
  linkStyle 7,8,9,10,11,12,13,14,15 stroke:#0000ff

  %% Link 16-20: Results cache initialization job --> Services
  results_cache_indices_creator -->|completed_successfully| api_server
  results_cache_indices_creator -->|completed_successfully| garbage_collector
  results_cache_indices_creator -->|completed_successfully| mcp_server
  results_cache_indices_creator -->|completed_successfully| reducer
  results_cache_indices_creator -->|completed_successfully| webui
  linkStyle 16,17,18,19,20 stroke:#008000

  subgraph Databases
    database
    results_cache
    subgraph celery_dependencies[Celery Dependencies]
        queue
        redis
    end
  end

  subgraph Initialization jobs
    db_table_creator
    results_cache_indices_creator
  end

  subgraph Schedulers
    compression_scheduler
    query_scheduler
    spider_scheduler
  end

  subgraph Workers
    compression_worker
    spider_compression_worker
    query_worker
    reducer
  end

  subgraph Management & UI
    api_server
    log_ingestor
    garbage_collector
    webui
  end

  subgraph AI
    mcp_server
  end

  %% Subgraph styles
  style celery_dependencies fill:#ffffe0
  style spider_compression_worker fill:#008080
  style spider_scheduler fill:#008080

+++
**Figure 1**: Orchestration architecture of the services in the CLP package.
::::

(table-1)=
::::{card}

:::{table}
:align: left

| Service                   | Description                                                        |
|---------------------------|--------------------------------------------------------------------|
| database                  | Database for archive metadata, compression jobs, and query jobs    |
| queue                     | Task queue for schedulers                                          |
| redis                     | Task result storage for workers                                    |
| compression_scheduler     | Scheduler for compression jobs                                     |
| query_scheduler           | Scheduler for search/aggregation jobs                              |
| spider_scheduler          | Scheduler for Spider distributed task execution framework          |
| results_cache             | Storage for the workers to return search results to the UI         |
| compression_worker        | Worker processes for compression jobs using Celery                 |
| spider_compression_worker | Worker processes for compression jobs using Spider                 |
| query_worker              | Worker processes for search/aggregation jobs using Celery          |
| reducer                   | Reducers for performing the final stages of aggregation jobs       |
| api_server                | API server for submitting queries                                  |
| webui                     | Web server for the UI                                              |
| mcp_server                | MCP server for AI agent to access CLP functionalities              |
| garbage_collector         | Process to manage data retention                                   |
| log_ingestor              | Server for orchestrating and running continuous log ingestion jobs |

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

## Orchestration methods

CLP supports two orchestration methods: Docker Compose for single-host or manual multi-host
deployments, and Helm for Kubernetes deployments. Both methods share the same configuration
interface (`clp-config.yaml` and `credentials.yaml`) and support the same deployment types.

### Configuration

Each service requires configuration values passed through config files, environment variables,
and/or command line arguments. Since services run in containers, some values must be adapted for the
orchestration environment. Specifically, host paths must be converted to container paths, and
hostnames/ports must use service discovery mechanisms.

The orchestration controller (e.g., `DockerComposeController`) reads `etc/clp-config.yaml` and
`etc/credentials.yaml`, then generates:

* A container-specific CLP config file with adapted paths and service names
* Runtime configuration (environment variables or ConfigMaps)
* Required directories (e.g., data output directories)

For Docker Compose, this generates `var/log/.clp-config.yaml` and `.env`. For Kubernetes, the Helm
chart generates a ConfigMap and Secrets from `values.yaml`.

:::{note}
We are currently developing a `KubernetesController`, which will unify the configuration experience
across both orchestration methods. The new controller will read `clp-config.yaml` and
`credentials.yaml` like `DockerComposeController`, then set up the Helm release accordingly.
:::

### Secrets

Sensitive credentials (database passwords, API keys) are stored in `etc/credentials.yaml` and
require special handling to avoid exposure.

* **Docker Compose**: Credentials are written to `.env` and passed as environment variables
* **Kubernetes**: Credentials are stored in Kubernetes Secrets

### Dependencies

As shown in [Figure 1](#figure-1), services have complex interdependencies. Both orchestrators
ensure services start only after their dependencies are healthy.

* **Docker Compose**: Uses `depends_on` with `condition: service_healthy` and container healthchecks
* **Kubernetes**: Uses init containers (via the `clp.waitFor` helper) and readiness/liveness probes

### Storage

Services require persistent storage for logs, data, archives, and streams.

* **Docker Compose**: Uses bind mounts for host directories and named volumes for database data.
  Conditional mounts use variable interpolation to mount empty tmpfs when not needed.
* **Kubernetes**: Uses dynamically provisioned PersistentVolumeClaims for persistent data (database,
  results cache, archives, streams) and `emptyDir` volumes for ephemeral state (Redis, staging
  directories). Service logs are emitted to pod stdout/stderr.

### Deployment types

CLP supports multiple deployment configurations based on the compression scheduler and query engine.

| Deployment Type | Compression Scheduler | Query Engine                 |
|-----------------|-----------------------|------------------------------|
| Base            | Celery                | [Presto][presto-integration] |
| Full            | Celery                | Native                       |
| Spider Base     | Spider                | [Presto][presto-integration] |
| Spider Full     | Spider                | Native                       |

:::{note}
Spider support is not yet available for Helm.
:::

Docker Compose selects the appropriate compose file (e.g., `docker-compose.yaml` for Full,
`docker-compose-spider.yaml` for Spider Full) and uses `deploy.replicas` with environment
variables (e.g., `CLP_MCP_SERVER_ENABLED`) to toggle optional services. Helm uses conditional
templating to include/exclude resources.

## Troubleshooting

When issues arise, use the appropriate commands for your orchestration method:

* [Docker Compose debugging][docker-compose-debugging]
* [Kubernetes Helm debugging][kubernetes-debugging]

## User guides

* [Kubernetes deployment][kubernetes-guide]: Deploying CLP with Helm
* [Multi-host deployment][docker-compose-multi-host]: Manual Docker Compose across multiple hosts

[docker-compose]: https://docs.docker.com/compose/
[docker-compose-debugging]: ../user-docs/guides-docker-compose-deployment.md#monitoring-and-debugging
[helm]: https://helm.sh/
[kubernetes]: https://kubernetes.io/
[kubernetes-debugging]: ../user-docs/guides-k8s-deployment.md#monitoring-and-debugging
[kubernetes-guide]: ../user-docs/guides-k8s-deployment.md
[docker-compose-multi-host]: ../user-docs/guides-docker-compose-deployment.md#multi-host-deployment
[presto-integration]: ../user-docs/guides-using-presto.md
