# Overview

These guides cover deploying, configuring, and integrating CLP for various use cases.

---

## Deployment

Guides for deploying CLP in production environments.

:::{tip}
For single-host deployments, see the [quick-start guide](quick-start/index), which includes tabs
for both Docker Compose and Kubernetes (kind) orchestration.
:::

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: guides-docker-compose-deployment
Docker Compose deployment
^^^
Deploy CLP using Docker Compose for single or multi-host setups.
:::

:::{grid-item-card}
:link: guides-k8s-deployment
Kubernetes deployment
^^^
Deploy CLP on a Kubernetes cluster using Helm.
:::
::::

---

## Input & storage

Guides for configuring data sources and storage backends.

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: guides-using-object-storage/index
Using object storage
^^^
Ingest logs from and store archives on S3-compatible object storage.
:::

:::{grid-item-card}
:link: guides-external-database
External database setup
^^^
Use external MariaDB/MySQL and MongoDB databases.
:::

:::{grid-item-card}
:link: guides-retention
Configuring retention periods
^^^
Configure retention periods for archives and search results.
:::
::::

---

## Package services

Guides for using services included in the CLP package.

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: guides-using-the-api-server
Using the API server
^^^
Submit queries, view results, and manage jobs programmatically.
:::

:::{grid-item-card}
:link: guides-mcp-server/index
MCP server
^^^
Integrate CLP with AI assistants using the Model Context Protocol.
:::

:::{grid-item-card}
:link: guides-using-log-ingestor
Using `log-ingestor`
^^^
How to use `log-ingestor` to continuously ingest logs.
:::

:::{grid-item-card}
:link: guides-using-presto
Using Presto
^^^
Use Presto for distributed SQL queries on compressed logs.
:::

:::{grid-item-card}
:link: guides-using-spider
Using Spider
^^^
Use Spider for compression and query job task distribution.
:::
::::
