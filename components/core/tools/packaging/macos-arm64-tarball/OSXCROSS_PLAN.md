# Dockerized OSXCross macOS Tarball Build Notes

## Summary

This directory contains a local containerized Linux-to-macOS cross-build path
that lets developers run one command to produce a self-contained
`clp-core-<version>-macos-<arch>.tar.gz` tarball for `arm64` or `x86_64`.
The public entrypoint is
`build.sh`; by default it uses the Docker/OSXCross route and leaves native
macOS builds behind the explicit `--native` flag.

Use a local CLP toolchain image derived from a prebuilt OSXCross image and
continue shipping bundled dynamic `.dylib` files. Do not pursue a fully static
macOS binary because CLP disables static linking on Apple platforms and
MariaDBClient is intentionally dynamic.

## Implemented Shape

- `build.sh` is the front door. It defaults to Docker/OSXCross, forwards common
  build options, and dispatches native host builds to `build-native.sh` only
  when `--native` is supplied.
- `build-cross.sh` builds/runs a local Docker image derived from the configured
  OSXCross base image and writes the tarball to `packages/` by default.
- The shared `build-package.sh` pipeline performs version resolution,
  build-family isolation, submodule initialization, `task deps:core`, optional
  post-dependency hooks, CMake configure/build, and tarball packaging.
- A local-only Docker image is layered on top of
  `ghcr.io/crazy-max/osxcross:14.5-r0-ubuntu` by copying its `/osxcross`
  toolchain/SDK into an Ubuntu runtime image with CMake, Task, Python,
  pkg-config, Mach-O tools, MacPorts dependencies, and MariaDB Connector/C.
- The local flow does not accept a macOS SDK tarball; the prebuilt OSXCross base
  image supplies the SDK/toolchain.
- Docker uses the server's Linux platform by default, while allowing
  `--builder-platform` to force a platform for local testing.
- CLP dependencies are cross-built with the OSXCross toolchain before configuring
  CLP itself.
- Cross CMake flags are injected from the container wrapper so the
  yscope-dev-utils submodule does not need to be modified.
- The package reuses the existing macOS tarball layout: `bin/` for executables
  and `lib/clp/` for bundled dylibs.
- `bundle-dylibs.sh` supports Linux by parameterizing Mach-O tools:
  `otool`, `install_name_tool`, `codesign`, and `strip` resolve to OSXCross
  tools or a Linux-compatible signer when cross-building.
- The OSXCross image builds a pinned native `ldid` and uses it to ad-hoc sign
  Mach-O files after install-name/RPATH rewrites.
- Packaging fails if no Mach-O signing tool is available, unless an explicit
  local-test-only unsigned override is set.
- `ldid` is treated as a build-time-only tool: it is AGPL-3.0 licensed and must
  not be copied into the generated CLP tarball.
- Signing is intentionally tiered: Linux-side `ldid` output is the default
  local/internal artifact, optional macOS host ad-hoc re-signing can provide an
  Apple-recognized local signature, and Developer ID signing plus notarization
  is deferred for a future public-release workflow.
- The implementation prefers the existing `@rpath` / `@loader_path` layout over
  a flat `@executable_path` layout: binaries use `@loader_path/../lib/clp`;
  dylibs use `@loader_path` for siblings.

## Dependency Strategy

- First try building CLP's existing source-built dependency graph under OSXCross,
  because `task deps:core` already pins versions and generates CMake settings.
- Use `osxcross-macports` for dependencies that are currently expected from
  Homebrew/system packages.
- Install architecture-specific MacPorts dependencies with the helper's
  supported macOS 13.0 deployment target while keeping the final CLP build
  target at macOS 14.0.
- Keep the final minimum macOS version in `defaults.sh` and expose
  `--deployment-target` / `MACOSX_DEPLOYMENT_TARGET` overrides. The SDK version
  controls available headers/stubs, not the runtime floor.
- Resolve libcurl from the bundled macOS SDK instead of MacPorts; the SDK
  provides the libcurl headers and `.tbd` linker stub needed by CMake.
- Verify availability and versions for `libarchive`, `mariadb-connector-c`,
  SDK libcurl, and `openssl` before relying on the local image.
- Keep dependency artifacts isolated under build families such as
  `build-macos-arm64-cross/` and `build-macos-x86_64-cross/` so they do not
  collide with native macOS or Linux package builds.

## Implementation Status

- Completed: build a minimal OSXCross Docker image and prove it can compile a
  small Mach-O binary plus one linked dylib.
- Completed: prove install-name rewriting and ad-hoc signing inside Linux using
  OSXCross tools plus `ldid`, then validate the result on an actual arm64
  macOS host.
- Completed: thread cross-toolchain CMake flags through CLP's dependency build
  flow and build `task deps:core`.
- Completed: configure and build CLP with `CLP_USE_STATIC_LIBS=OFF`, package with
  the adapted dylib bundler, and emit the tarball.
- Completed: document the single-command flow and known limitations.

## Validation Checklist

- Run the Dockerized command from a clean checkout and verify it creates exactly
  one `clp-core-*-macos-<arch>.tar.gz`.
- Inspect the tarball contents and verify it contains `bin/` executables plus
  bundled dylibs under `lib/clp/`.
- Use OSXCross `otool -L` to verify no bundled binary or dylib references
  `/opt/homebrew`, the Linux build path, or other absolute non-system dependency
  paths.
- Transfer the tarball to a matching-architecture macOS host and run `clp --help`,
  `clp-s --help`, `clo --help`, `clg --help`, `indexer --help`,
  `log-converter --help`, and `reducer-server --help`.
- Validate that the tarball works on a macOS host without Homebrew-installed CLP
  dependencies.
- Confirm existing native macOS packaging and Linux `.deb/.rpm/.apk`
  packaging still work.

## Constraints

- Supported target architectures are `arm64` and `x86_64`.
- Default deployment target is macOS 14.0 (Sonoma).
- Distribution remains a self-contained `.tar.gz`, not a `.pkg`, `.dmg`, or
  Homebrew tap.
- Dynamic dylib bundling is acceptable and preferred.
- Release validation still requires a real macOS host for each target
  architecture because Linux cannot execute the produced Mach-O binaries.
- CI integration is out of scope until the local Dockerized flow is proven.
