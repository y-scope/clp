# Docker Compose design

This document explains the technical details of CLP's Docker Compose implementation.

## Overview

The Docker Compose implementation depends on a new controller architecture with a `BaseController` 
abstract class and a `DockerComposeController` implementation.

## Architecture

### Controller

The implementation uses a controller pattern:

* `BaseController` (abstract): Defines the interface for provisioning and managing CLP components.
* `DockerComposeController`: Implements the Docker Compose-specific logic.

### Key Components

1. **Provisioning Methods**: Each CLP component has a dedicated provisioning method in the controller:
   * `provision_database()`
   * `provision_queue()`
   * `provision_redis()`
   * `provision_results_cache()`
   * `provision_compression_scheduler()`
   * `provision_query_scheduler()`
   * `provision_compression_worker()`
   * `provision_query_worker()`
   * `provision_reducer()`
   * `provision_webui()`
   * `provision_garbage_collector()`

2. **Environment Generation**: The controller generates a `.env` file with all necessary environment 
   variables for Docker Compose.

3. **Configuration Transformation**: The `transform_for_container_config()` method in `CLPConfig`
   and related classes adapts the configuration for containerized environments.

## Docker Compose File

The `docker-compose.yml` file defines all services with:

* Proper service dependencies using `depends_on`
* Health checks for critical services
* Volume mounts for persistent data
* Network configuration
* User permissions
* Resource limits

## Deployment Process

1. **Configuration Loading**: The start script loads and validates the CLP configuration.
2. **Provisioning**: The controller provisions all components and generates environment variables.
3. **Environment File Generation**: A `.env` file is created with all necessary variables.
4. **Docker Compose Execution**: `docker compose up -d` is executed to start all services.

## Service Architecture

The Docker Compose setup includes the following services:

* **database**: MySQL/MariaDB for metadata storage
* **queue**: RabbitMQ for job queuing
* **redis**: Redis for task result storage
* **results-cache**: MongoDB for search results caching
* **compression-scheduler**: Schedules compression jobs
* **query-scheduler**: Schedules search jobs
* **compression-worker**: Executes compression tasks
* **query-worker**: Executes search tasks
* **reducer**: Handles aggregation operations
* **webui**: Web interface for CLP
* **garbage-collector**: Manages retention policies
* **db-table-creator**: Initializes database tables
* **results-cache-indices-creator**: Sets up MongoDB indices

## Service Dependencies

Docker Compose manages service startup order through:

* `depends_on` directives.
* Health checks with `condition: service_healthy`.
* Init containers for one-time setup tasks (e.g., `db-table-creator`).

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
