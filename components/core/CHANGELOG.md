# Change Log
A list of notable changes, especially those that affect usage of the software.

# 2021-Sep-17
* fmtlib added as dependency
* Minimum required version of spdlog has been increased to match version of
  fmtlib
  * If you installed an older version of spdlog using the provided script,
    you will need to remove it (`dpkg -r libspdlog-dev`) and install the new version.

# 2021-Sep-24
* CLP can now run with a MariaDB database server allowing parallel compression to the same 
  directory.
* CLP cannot statically link MariaDB due to its license, so the MariaDB C client must be 
  installed wherever CLP is run.
* json and yaml-cpp were added as submodule dependencies.
