# Docker Compose design

This document explains the technical details of CLP's Docker Compose implementation.

## Overview

The Docker Compose implementation depends on a new controller architecture with a `BaseController`
abstract class and a `DockerComposeController` implementation.

## Architecture

### Controller

The orchestration implementation uses a controller pattern:

* `BaseController` (abstract): Defines the interface for provisioning and managing CLP components.
* `DockerComposeController`: Implements the Docker Compose-specific logic.

### Initialization

1. **Provisioning Methods**: Each CLP component has a dedicated provisioning method in the 
   controller: `provision_<component-name>()`.
2. **Environment Generation**: The controller generates a `.env` file with all necessary environment 
   variables for Docker Compose.
3. **Configuration Transformation**: The `transform_for_container_config()` method in `CLPConfig`
   and related classes adapts the configuration for containerized environments.

## Docker Compose File

The `docker-compose.yaml` file defines all services with:

* Proper service dependencies using `depends_on`
* Health checks for critical services
* Volume mounts for persistent data
* Network configuration
* User permissions
* Resource limits

## Deployment Process

The `start-clp.py` script performs the following steps:

1. **Configuration Loading**: The start script loads and validates the CLP configuration.
2. **Provisioning**: The controller provisions all components and generates environment variables.
3. **Environment File Generation**: A `.env` file is created with all necessary variables.
4. **Docker Compose Execution**: `docker compose up -d` is executed to start all services.

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

The CLP package is composed of several service components. The tables below list the services and their functions.

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
