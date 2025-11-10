# Dependency Taskfiles

Follow the guidelines below when writing or updating task-based dependency installation files.

* Use one of the following tasks for CMake library installation, in descending order of preference:
  * `deps:utils:install-remote-cmake-lib`
  * `yscope-dev-utils:cmake:install-remote-tar`

* For special cases where the above tasks are not applicable:
  * If the library can be installed via CMake but a custom approach is used, briefly document why.
  * Include `deps:utils:init` in the `deps` section of the task.
  * Ensure each library installation includes checksum validation to the best extent possible.
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
    `OLD`).
  * The component depends on another via `<lib_name>_ROOT`.

* Build static-only libraries whenever possible:
  * If a library produces both static and shared artifacts by default, disable shared artifact
    building to ensure consistent `find_package()` behavior.
  * If shared artifacts cannot be disabled, explicitly link CLP targets against the static export
    target (e.g., `mongo::mongocxx_static`).
  * If a library cannot be statically linked (e.g., MariaDBClient due to GPL licensing), briefly
    document the reason.

* Skip unit tests, examples, docs, or unused binaries to speed up the installation process.

* Prefer disabling lib-specific testing flags (e.g., `CATCH_BUILD_TESTING=OFF`) rather than setting
  the generic `BUILD_TESTING` to `OFF`.

* **Lastly**, while satisfying the above requirements, remove redundant flags that merely restate
  default values found in `CMakeLists.txt` and other source CMake files, with two exceptions:
  * Set `CMAKE_BUILD_TYPE` to `Release` unless another build type is explicitly required.
  * Set `CMAKE_INSTALL_MESSAGE` to `LAZY` to reduce log verbosity during installation.
