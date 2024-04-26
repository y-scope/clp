# CLP for unstructured logs

There are two flavours of CLP for compressing unstructured logs.

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: clp
clp
^^^
A flavour of CLP that stores logs using a simple three column format. 
:::

:::{grid-item-card}
:link: glt
glt
^^^
An experimental flavour of CLP that stores logs grouped by their log type. 
:::
::::

CLP parses unstructured logs by using a set of regexes for parsing variable
values. CLP's schema file allows you configure these regexes.

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: schema
Schema file syntax
^^^
The syntax of CLP's schema file. 
:::
::::

:::{toctree}
:hidden:
clp
glt
schema
:::
