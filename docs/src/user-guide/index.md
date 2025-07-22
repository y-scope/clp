# User guide

This guide documents how to use and operate CLP. Use the left sidebar (if it's hidden, click the
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
:link: reference-json-search-syntax
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
guides-using-object-storage/index
guides-multi-node
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

reference-json-search-syntax
reference-text-search-syntax
reference-unstructured-schema-file
:::
