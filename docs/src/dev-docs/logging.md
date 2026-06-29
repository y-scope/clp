# CLP package logging

The CLP package utilizes a polyglot architecture, meaning each component family manages its own logging stack. There is **no single project-wide JSON schema**.

This document is divided into two sections:

* [Developer guide](logging-developer-guide.md): How to set up and write logs when modifying CLP components.
* [Operator guide](logging-operator-guide.md): How to configure log levels, capture service logs, and understand the component-specific log structures in deployment.

:::{toctree}
:hidden:

logging-developer-guide
logging-operator-guide
:::
