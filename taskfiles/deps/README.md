# Dependency Taskfiles

Follow the guidelines below when writing or updating dependency installation taskfiles.

* Use one of the following tasks for installation, in descending order of preference:
  * `deps:utils:install-remote-cmake-lib`
  * `yscope-dev-utils:cmake:install-remote-tar`

* For special tasks that don't apply to the rule above:
  * Include `deps:utils:init` in the `deps` section of the task.
  * Briefly explain why if the library is installable via CMake.
  * Ensure each library installation includes checksum validation using the best effort.
    * Use of `yscope-dev-utils` tasks is encouraged, as most of them include proper checksum checks.
  * Use `<lib>-extracted` as the directory name for tarball extractions.

* Avoid parsing version numbers in download URLs unless the version is used elsewhere, as URL
  formats can change over time, making it more maintainable to store and use the full URL directly
  as plain text.

## CMake Generate Arguments

Apply the following guidelines to the `CMAKE_GEN_ARGS` section of tasks that call
`deps:utils:install-remote-cmake-lib`:

* Set `CMP0074` to `NEW` whenever:
  * The component's minimum required CMake version is less than 3.27 (where `CMP0074` defaults to
    `OLD`)
  * The component depends on another via `<lib_name>_ROOT`.

* Build both shared and static libraries when possible.

* Skip unit tests, examples, docs or unused binaries to speed up the installation process.

* Prefer disabling lib-specific testing flags (e.g., `CATCH_BUILD_TESTING=OFF`) rather than setting
  the generic `BUILD_TESTING` to `OFF`.

* **Lastly**, while satisfying the above requirements, remove redundant flags that merely restate
  default values found in `CMakeLists.txt` and others source CMake files, with two exceptions:
  * Set `CMAKE_BUILD_TYPE` to `Release` unless another build type is explicitly required.
  * Set `CMAKE_INSTALL_MESSAGE` to `LAZY` to reduce log verbosity during installation.
