# Docker Compose design

This document explains the technical details of CLP's Docker Compose implementation.

## Overview

The Docker Compose implementation follows a controller architecture with a `BaseController` abstract
class and a `DockerComposeController` implementation.

## Architecture

### Controller Pattern

The orchestration uses a controller pattern:

* `BaseController` (abstract): Defines the interface for provisioning and managing CLP components.
* `DockerComposeController`: Implements Docker Compose-specific logic.

## Initialization

The controller performs these initialization steps:

1. **Provisioning**: Provisions all components and generates component specific configuration
   variables.
2. **Configuration Transformation**: The `transform_for_container()` method in `CLPConfig` adapts
   configurations for containerized environments
3. **Environment Generation**: Creates a `.env` file with necessary Docker Compose variables  

### Configuration Transformation

The `transform_for_container()` method in the `CLPConfig` class and related component classes
adapts the configuration for containerized environments by:

1. Converting host paths to container paths
2. Updating service hostnames to match Docker Compose service names
3. Setting appropriate ports for container communication

### Environment Variables

The controller generates a comprehensive set of environment variables that are written to a `.env`
file, including:

* Component-specific settings (ports, logging levels, concurrency)
* Credentials for database, queue, and Redis services
* Paths for data, logs, archives, and streams
* AWS credentials when needed

## Deployment Process

The `start-clp.sh` script executes the `start_clp.py` Python script to orchestrate the deployment.

### Deployment Types

CLP supports two deployment types determined by the `package.query_engine` configuration setting:

1. **BASE**: For deployments using [Presto][presto-integration] as the query engine. Uses only
   `docker-compose.base.yaml`.
2. **FULL**: For deployments using CLP's native query engine. Uses both compose files.

## Docker Compose Files

The Docker Compose setup uses two files:

* `docker-compose.base.yaml`: Defines base services for all deployment types, excluding Celery
  scheduler and worker components to allow separate Presto [integration][presto-integration].
* `docker-compose.yaml`: Extends the base file with additional services for complete deployments

Each file defines services with:

* Service dependencies via `depends_on`
* Health checks for critical services
* Volume binding mounts for persistent data
* Network configuration
* User permissions

### Health check defaults

Below are the default health check settings and the rationale for each:

* `interval: 30s` — default probe interval in steady state. Avoid setting this too low to avoid 
  excessive resource usage.
* `timeout: 2s` — no remote communication is expected, so a short timeout is sufficient.
* `retries: 3`
  - a service is deemed unhealthy if it does not respond in ~(30s * 3) = 90s since it is in the
    steady state.
  - a service is deemed unhealthy if it does not respond within ~(60s + 90s) since it is started.
* `start_interval: 2s` — A short interval allows the service to become healthy quickly once it 
  is ready, allowing other services which depend on it to start.
* `start_period: 60s` — the first minute of startup ignores failures, effectively granting around 30
  fast attempts to become healthy before retries start counting.

## Service architecture

The Docker Compose setup includes the following services:

:::{mermaid}
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
  db_table_creator -->|completed_successfully| garbage_collector
  results_cache_indices_creator -->|completed_successfully| garbage_collector

  subgraph Databases
    database
    queue
    redis
    results_cache
  end

  subgraph DB Migration Jobs
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
:::

### Services overview

The CLP package is composed of several service components. The tables below list the services and
their functions.

:::{table} Services
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
| garbage_collector     | Background process for retention control                        |
:::

### One-time initialization jobs

We also set up short-lived run-once "services" to initialize some services listed above.

:::{table} Initialization jobs
:align: left

| Job                           | Description                                             |
|-------------------------------|---------------------------------------------------------|
| db-table-creator              | Initializes database tables                             |
| results-cache-indices-creator | Initializes single-node replica set and sets up indices |
:::

## Troubleshooting

If you encounter issues with the Docker Compose deployment:

1. Check service status:
   ```bash
   docker compose ps
   ```

2. View service logs:
   ```bash
   docker compose logs <service-name>
   ```

3. Validate configuration:
   ```bash
   docker compose config
   ```

[presto-integration]: ../user-docs/guides-using-presto.md
