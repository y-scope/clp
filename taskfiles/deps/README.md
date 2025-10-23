# Dependency Taskfiles

Follow the guidelines below when writing and updating dependency installation taskfiles.

- Use `deps:utils:install-remote-cmake-lib` and thus `yscope-dev-utils:cmake:install-remote-tar`
  whenever it applies. For respective task of other libraries/components:
  - Include `deps:utils:init` in the `deps:` section of the task.
  - Explain why we aren't installing it if it is a CMake project that's installable.
  - Ensure that they have proper checksum mechanisms to check if they are up-to-date.
  - Use `<lib>-extracted` as the folder name for the contents extracted from the downloaded tarball.

- Avoid parsing the version number from the download URL unless it is needed elsewhere. Since URL
  formats can change over time, it’s more maintainable to use the full URL directly as plain text.

## CMake Generate Arguments

The following guidelines apply to flags in the `CMAKE_GEN_ARGS` section of tasks that call
`deps:utils:install-remote-cmake-lib`:

- Set `CMP0074` to `NEW` in `CMAKE_GEN_ARGS` whenever:
  - The component's minimum required CMake version is less than 3.27 (where CMP0074 defaults to OLD)
  - The component depends on another via `<lib_name>_ROOT`.

- Attempt to build both shared and static libraries for each dependency.

- Avoid building unit tests, examples, docs or binaries that are unused to speed up the install
  process.

- Choose to turn off lib-specific build testing flag instead of `BUILD_TESTING` if possible.

- Set `CMAKE_BUILD_TYPE` to `Release` if another build type is not strictly required.

- Set `CMAKE_INSTALL_MESSAGE` to `LAZY` to avoid clobbering of the log screen.

- Do not include flags that are no-op, including the ones that merely set variables to their
  default values found in `CMakeLists.txt` and other cmake files used to generate `CMakeCache.txt`.
