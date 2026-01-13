# User docs

These docs explain how to use and operate CLP. Use the left sidebar (if it's hidden, click the
<i class="fa fa-bars"></i> icon) to navigate to specific docs.

## Quick start

New to CLP? Get CLP up and running quickly on a single host.

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: quick-start/index
Quick start
^^^
A quick-start guide for choosing a flavor of CLP, setting it up, compressing your logs, and
searching them.
:::
::::

---

## Multi-host Deployment

Guides for deploying CLP in multi-host environments for production ready use cases.

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

Configure how CLP ingests, stores, and retains data.

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: guides-using-object-storage/index
Using object storage
^^^
Ingest logs from and store archives on S3-compatible object storage.
:::

:::{grid-item-card}
:link: guides-using-log-ingestor
Using `log-ingestor`
^^^
How to use `log-ingestor` to continuously ingest logs.
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

Learn how to interact with CLP package services.

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

---

## Core

CLP's core component contains the code to compress, decompress, and search logs. You can use it to
try CLP on your logs before deploying a cluster.

There are currently two flavours of CLP, depending on the format of your logs:

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: core-unstructured/index
CLP for unstructured logs
^^^
A flavour of CLP for unstructured (e.g., free-text) logs.
:::

:::{grid-item-card}
:link: core-clp-s
CLP for JSON logs
^^^
A flavour of CLP for JSON logs.
:::
::::

The guide below explains how to use a container to try either flavour.

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: core-container
Core container
^^^
A container for trying CLP's core.
:::
::::

---

## Resources

Sample datasets and other resources for testing and benchmarking CLP.

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: resources-datasets
Datasets
^^^
Log datasets for testing and benchmarking.
:::
::::

---

## Reference

Syntax and tools for working with compressed logs.

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: reference-json-search-syntax
`clp-json` search syntax
^^^
Syntax reference for `clp-json`'s (and `clp-s`') search syntax.
:::

:::{grid-item-card}
:link: reference-text-search-syntax
`clp-text` search syntax
^^^
Syntax reference for `clp-text`'s (and `clp`'s) search syntax.
:::

:::{grid-item-card}
:link: reference-admin-tools
Admin tools
^^^
Reference for a set of tools for managing compressed logs in `clp-json` or `clp-text`.
:::

:::{grid-item-card}
:link: reference-unstructured-schema-file
Schema file syntax
^^^
Syntax reference for clp's schema file for parsing unstructured text logs.
:::
::::

:::{toctree}
:hidden:
:caption: Quick start

quick-start/index
quick-start/clp-json
quick-start/clp-text
:::

:::{toctree}
:hidden:
:caption: Deployment

guides-docker-compose-deployment
guides-k8s-deployment
:::

:::{toctree}
:hidden:
:caption: Input & storage

guides-using-object-storage/index
guides-using-log-ingestor
guides-external-database
guides-retention
:::

:::{toctree}
:hidden:
:caption: Package services

guides-using-the-api-server
guides-mcp-server/index
guides-using-presto
guides-using-spider
:::

:::{toctree}
:hidden:
:caption: Core

core-container
core-clp-s
core-unstructured/index
:::

:::{toctree}
:hidden:
:caption: Resources

resources-datasets
:::

:::{toctree}
:hidden:
:caption: Reference

reference-json-search-syntax
reference-text-search-syntax
reference-admin-tools
reference-unstructured-schema-file
:::
