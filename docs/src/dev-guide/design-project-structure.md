# Project structure

CLP is currently split across a few different components in the [components][1] directory:

* [clp-package-utils][2] contains Python utilities for operating the CLP package.
* [clp-py-utils][3] contains Python utilities common to several of the other components.
* [core][4] contains code to compress uncompressed logs, decompress compressed logs, and search
  compressed logs.
* [job-orchestration][5] contains code to schedule compression jobs on the cluster.
* [package-template][6] contains the base directory structure and files of the CLP package.
* [webui][7] contains the web interface for the CLP package.

[1]: https://github.com/y-scope/clp/tree/main/components
[2]: https://github.com/y-scope/clp/tree/main/components/clp-package-utils
[3]: https://github.com/y-scope/clp/tree/main/components/clp-py-utils
[4]: https://github.com/y-scope/clp/tree/main/components/core
[5]: https://github.com/y-scope/clp/tree/main/components/job-orchestration
[6]: https://github.com/y-scope/clp/tree/main/components/package-template
[7]: https://github.com/y-scope/clp/tree/main/components/webui
