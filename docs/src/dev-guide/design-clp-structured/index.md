# clp-s

The compressed-log-processor-structured (abbreviated as clp-s and also known as [μSlope][μSlope])
archive format is a storage format for large chunks of dynamically structured (e.g., JSON) logs.
Compared to the [KV-IR](../design-kv-ir-streams/index.md) streaming compression format clp-s
achieves higher compression ratios and significantly faster search speeds at the cost of needing to
aggregate log data before compressing the data into archives and requiring memory proportional to
archive size for both compression and search. This makes clp-s unsuitable for real-time or severely
resource-constrained usage but ideal for long-term archival and search.

::::{grid} 1 1 1 1
:gutter: 2

:::{grid-item-card}
:link: background
Background
^^^
Any necessary information to understand clp-s' format.
:::

:::{grid-item-card}
:link: single-file-archive-format
Single-file archive format
^^^
A detailed reference for the single-file archive format.
:::
::::

:::{warning}
🚧 This section is still under construction.
:::

:::{toctree}
:hidden:

background
single-file-archive-format
:::

[μSlope]: https://www.usenix.org/conference/osdi24/presentation/wang-rui
