# KV-IR streams

The key-value pair internal representation (abbreviated as KV-IR) stream format is a storage format
for dynamically structured (e.g., JSON) logs. Compared to a JSON file, the KV-IR stream format is
smaller and faster for clp-s to compress. Compared to clp-s' archive format, the KV-IR stream format
buffers less data in-memory, making it more suitable for use in resource-constrained environments
and low-latency use cases (e.g., logging libraries). The pages in this section describe the format
as well as the key design decisions behind it.

::::{grid} 1 1 1 1
:gutter: 2

:::{grid-item-card}
:link: background
Background
:::

:::{grid-item-card}
:link: specification
Specificationz
:::
::::

:::{toctree}
:hidden:

background
specification
:::
