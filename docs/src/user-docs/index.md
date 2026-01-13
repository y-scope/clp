# User docs

These docs explain how to use and operate CLP. Use the left sidebar (if it's hidden, click the
<i class="fa fa-bars"></i> icon) to navigate to specific docs.

The sections are as follows:

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: quick-start/index
Quick start
^^^
A quick-start guide for choosing a flavor of CLP, setting it up, compressing your logs, and
searching them.
:::

:::{grid-item-card}
:link: guides-overview
Guides
^^^
Guides for using CLP in various use cases.
:::

:::{grid-item-card}
:link: core-overview
Core
^^^
Docs about CLP's core component for compressing, decompressing, and searching logs.
:::

:::{grid-item-card}
:link: resources-datasets
Resources
^^^
Resources like log datasets, etc.
:::

:::{grid-item-card}
:link: reference-overview
Reference
^^^
Reference docs like format specifications, etc.
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
:caption: Guides
:glob:

guides-overview
guides-mcp-server/index
guides-using-object-storage/index
guides-using-the-api-server
guides-using-log-ingestor
guides-external-database
guides-multi-host
guides-retention
guides-using-presto
guides-using-spider
:::

:::{toctree}
:hidden:
:caption: Core
:glob:

core-overview
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

reference-overview
reference-json-search-syntax
reference-text-search-syntax
reference-sbin-scripts/index
reference-unstructured-schema-file
:::
