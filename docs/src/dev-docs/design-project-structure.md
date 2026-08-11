# Project structure

CLP is currently split across a few different components in the [components] directory:

* [api-server] contains code for the API server of the CLP package.
* [clp-mcp-server] contains code for CLP MCP server.
* [clp-package-utils] contains Python utilities for operating the CLP package.
* [clp-py-utils] contains Python utilities common to several of the other Python components.
* [clp-rust-utils] contains Rust utilities common to several of the other Rust components.
* [clp-tdl-package] contains implementations of CLP tasks running on Spider.
* [compression-coordinator] contains code to coordinate CLP compression jobs running on Spider.
* [core] contains code to compress uncompressed logs, decompress compressed logs, and search
  compressed logs.
* [job-orchestration] contains code to schedule compression and search jobs on the cluster.
* [log-ingestor] contains code to serve requests for CLP ingestion job orchestration.
* [package-template] contains the base directory structure and files of the CLP package.
* [webui] contains the web interface for the CLP package.

[api-server]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/api-server
[components]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components
[clp-mcp-server]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/clp-mcp-server
[clp-package-utils]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/clp-package-utils
[clp-py-utils]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/clp-py-utils
[clp-rust-utils]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/clp-rust-utils
[clp-tdl-package]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/clp-tdl-package
[compression-coordinator]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/compression-coordinator
[core]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/core
[job-orchestration]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/job-orchestration
[log-ingestor]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/log-ingestor
[package-template]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/package-template
[webui]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/webui
