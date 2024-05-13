# Compression

You can compress your logs using a script in the CLP package. Depending on the format of your logs,
choose one of the options below.

:::{caution}
If you're using the `clp-json` release, you can only compress JSON logs. If you're using the
`clp-text` release, you should only compress unstructured text logs (`clp-text` can compress and
search JSON logs as if it was unstructured text, but `clp-text` cannot query individual fields).
This limitation will be addressed in a future version of CLP.
:::

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: json
Compressing JSON logs
:::

:::{grid-item-card}
:link: text
Compressing unstructured text logs
:::
::::

:::{toctree}
:hidden:
:caption: Compression

json
text
:::
