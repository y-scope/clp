# Project structure

CLP is currently split across a few different components in the [components] directory:

* [clp-mcp-server] contains code for CLP MCP Server.
* [clp-package-utils] contains Python utilities for operating the CLP package.
* [clp-py-utils] contains Python utilities common to several of the other components.
* [core] contains code to compress uncompressed logs, decompress compressed logs, and search
  compressed logs.
* [job-orchestration] contains code to schedule compression jobs on the cluster.
* [package-template] contains the base directory structure and files of the CLP package.
* [webui] contains the web interface for the CLP package.

[components]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components
[clp-mcp-server]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/clp-mcp-server
[clp-package-utils]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/clp-package-utils
[clp-py-utils]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/clp-py-utils
[core]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/core
[job-orchestration]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/job-orchestration
[package-template]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/package-template
[webui]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/webui
