# macOS tarball

Builds a self-contained `.tar.gz` of CLP core binaries for macOS `arm64` or
`x86_64`. Mirrors the role of `universal-deb/`, `universal-rpm/`, and
`alpine-apk/`, with a Dockerized OSXCross local build path and a native macOS
fallback.

## Tarball layout

```
clp-core-<version>-macos-<arch>.tar.gz
└── clp-core-<version>/
    ├── bin/
    │   ├── clg, clo, clp, clp-s, indexer, log-converter, reducer-server
    └── lib/clp/
        └── libboost_*.dylib, libfmt.*.dylib, libarchive.*.dylib, ...
```

All bundled dylibs are install-name-rewritten to `@rpath/<basename>`;
binaries carry `@loader_path/../lib/clp` as an rpath. No Homebrew install
is required on the target machine.

## Install

The binaries reference their dylibs via `@loader_path/../lib/clp`, so
the `bin/` and `lib/clp/` siblings must stay together. All three options
below preserve that. Pick whichever fits your machine; (1) is the
recommended default.

### (1) Extract once, symlink into PATH (recommended)

System-wide:

```shell
sudo tar xzf clp-core-<version>-macos-<arch>.tar.gz -C /opt
sudo ln -sfn /opt/clp-core-<version> /opt/clp-core
for b in clg clo clp clp-s indexer log-converter reducer-server; do
    sudo ln -sfn /opt/clp-core/bin/$b /usr/local/bin/$b
done
```

Per-user (no sudo):

```shell
mkdir -p ~/.local/opt ~/.local/bin
tar xzf clp-core-<version>-macos-<arch>.tar.gz -C ~/.local/opt
for b in clg clo clp clp-s indexer log-converter reducer-server; do
    ln -sfn ~/.local/opt/clp-core-<version>/bin/$b ~/.local/bin/$b
done
# Add ~/.local/bin to PATH in ~/.zshrc if it isn't already.
```

Why symlinks work: macOS resolves `@loader_path` against the *real*
binary location (following the symlink), so the binaries still find
their sibling `lib/clp/` even when invoked via a symlink in
`/usr/local/bin/`. Upgrade by extracting a new versioned dir alongside
and repointing the `/opt/clp-core` symlink.

### (2) Extract anywhere, append to PATH

```shell
tar xzf clp-core-<version>-macos-<arch>.tar.gz -C ~/Applications
echo 'export PATH="$HOME/Applications/clp-core-<version>/bin:$PATH"' \
    >> ~/.zshrc
```

Lightest touch but pollutes shell `PATH` with a versioned dir; you'll
edit `.zshrc` again at the next upgrade.

### (3) Copy bin/ and lib/clp/ into existing system paths

```shell
sudo tar xzf clp-core-<version>-macos-<arch>.tar.gz -C /tmp
sudo cp -R /tmp/clp-core-<version>/bin/* /opt/homebrew/bin/
sudo mkdir -p /opt/homebrew/lib/clp
sudo cp -R /tmp/clp-core-<version>/lib/clp/* /opt/homebrew/lib/clp/
```

The rpath `@loader_path/../lib/clp` resolves from `/opt/homebrew/bin/`
to `/opt/homebrew/lib/clp/`, so this works — but you must keep the two
dirs in sync. Uninstall and upgrade are manual.

### Gatekeeper: first-run quarantine

The tarball is **not** notarized (no Apple Developer Program
membership), so on first run Gatekeeper will block each binary with:

> "clp" cannot be opened because the developer cannot be verified.

Clear the download-quarantine attribute once after extraction (do this
on the extracted tree before symlinking, or directly on the install
location):

```shell
xattr -dr com.apple.quarantine /opt/clp-core-<version>/
# or, for the per-user variant:
xattr -dr ~/.local/opt/clp-core-<version>/
```

This is the standard workaround for any unsigned macOS release
(kubectl, older Homebrew bottles, etc.).

### Signing model

The current packaging flow intentionally uses three signing tiers:

1. **Default local artifact: Linux-side `ldid` signing.** The Docker/OSXCross
   path applies lightweight Mach-O signatures with `ldid` after rewriting
   install names and rpaths. This is sufficient for normal local/internal
   tarball builds where the user extracts the archive, clears quarantine if
   needed, and runs the CLP binaries directly.
2. **Optional stronger local artifact: macOS ad-hoc re-signing.** If an
   Apple-recognized ad-hoc signature is needed, unpack the tarball on macOS,
   run `/usr/bin/codesign --force --sign -` over the bundled dylibs and
   executables, verify with `codesign --verify`, and repack. This does not
   require an Apple Developer account and covers nearly all non-public-release
   distribution cases.
3. **Deferred release-grade artifact: Developer ID signing and notarization.**
   Public distribution without first-run Gatekeeper friction requires an Apple
   Developer Program account, Developer ID signing, notarization, and usually
   stapling. That path is intentionally out of scope for this local packaging
   flow for now.

## Build

Use `build.sh` as the front door. By default it runs the Dockerized OSXCross
path, even on macOS, so the host machine only needs Docker and the checked-out
repo.

Run from the repo root:

```shell
components/core/tools/packaging/macos-arm64-tarball/build.sh
```

Output: `packages/clp-core-<version>-macos-arm64.tar.gz` by default, or
`packages/clp-core-<version>-macos-x86_64.tar.gz` with `--arch x86_64`.

Common options:

| Flag | Meaning |
| ---- | ------- |
| `--version VER` | Override package version (default: from `taskfile.yaml`) |
| `--output DIR`  | Override output directory (default: `<repo>/packages`) |
| `--cores N`     | Parallel build jobs |
| `--arch ARCH`   | Target architecture: `arm64` or `x86_64` (default: `arm64` for OSXCross, host architecture for native builds) |
| `--deployment-target VER` | Minimum macOS version for generated binaries (default: `14.0`, Sonoma) |
| `--clean`       | Remove the CLP build directory before building; also allows replacing existing real `build/` / `.task/` directories with build-family symlinks |
| `--native`      | Build directly on a Mac instead of Docker/OSXCross |

Version handling matches the existing Linux packaging flow. Release versions
without a hyphen are preserved. Hyphenated pre-release versions such as
`0.12.1-dev`, `0.12.1-rc1`, or a hyphenated value passed with `--version` are
normalized to a reproducible source snapshot:
`<base-version>-<git-date>.<git-hash>`.

The minimum runtime macOS version is controlled by the deployment target, not by
the SDK version. The default is macOS 14.0 (Sonoma), defined in `defaults.sh`.
Override it with `--deployment-target <version>` or
`MACOSX_DEPLOYMENT_TARGET=<version>`.

### Recommended local build: Docker/OSXCross

This is the default `build.sh` route. It produces the same tarball format as the
native build, but runs in Docker with a local CLP toolchain image derived from a
prebuilt OSXCross image.

This local-dev path does not accept a macOS SDK tarball. The default base image
is `ghcr.io/crazy-max/osxcross:14.5-r0-ubuntu`, which supplies the OSXCross
toolchain with the macOS 14.5 SDK. The CLP image copies that toolchain into an
Ubuntu runtime image and layers on CMake, Task, `ldid`, MacPorts dependencies,
and MariaDB Connector/C. By default, the wrapper builds a local image tagged
`clp-macos-arm64-osxcross:dev`, mounts the repository, and writes
`packages/clp-core-<version>-macos-arm64.tar.gz`.
With `--arch x86_64`, the default image tag becomes
`clp-macos-x86_64-osxcross:dev` and the output suffix becomes
`macos-x86_64`. The image installs MacPorts dependencies for the requested
target architecture; libcurl is resolved from the bundled macOS SDK rather than
MacPorts.

`build.sh` is the public interface. The Docker image contains the toolchain and
build-time dependencies, while `build-in-osxcross-container.sh` configures the
per-run environment: compiler variables, pkg-config paths, CMake toolchain
wrapper, Boost cross-build options, Mach-O tooling, and packaging hooks.

Override the base image with `--base-image` or `OSXCROSS_BASE_IMAGE`; pin a tag
or digest if you need repeatable local builds.
By default, the wrapper targets the Docker server's Linux architecture (for
example `linux/arm64` on Apple Silicon Docker Desktop). Use `--builder-platform`
to force a different Linux platform.

Useful options:

```shell
components/core/tools/packaging/macos-arm64-tarball/build.sh \
  --base-image ghcr.io/crazy-max/osxcross:14.5-r0-ubuntu \
  --builder-platform linux/arm64 \
  --arch arm64 \
  --deployment-target 14.0 \
  --output /path/to/output-dir \
  --cores 8 \
  --clean
```

When `--output` is set, the path is resolved on the host and mounted into the
container, so absolute paths outside the repository are safe to use.

The first run can be slow and disk-heavy because it builds the local toolchain
image and the CLP C++ dependency graph. Subsequent runs can reuse the image and
incremental build directories:

```shell
components/core/tools/packaging/macos-arm64-tarball/build.sh --no-build-image
```

The build keeps macOS artifacts in family-specific directories such as
`build-macos-arm64-cross/` or `build-macos-x86_64-cross/` and points the
repo-level `build/` and `.task/` paths at them with symlinks. If either
repo-level path already exists as a real directory, the build refuses to replace
it unless `--clean` is supplied.

In corporate or otherwise restricted Docker networking environments, set
`DOCKER_NETWORK` to the network Docker should use. The wrapper forwards it to
both `docker build` and `docker run`:

```shell
DOCKER_NETWORK=host components/core/tools/packaging/macos-arm64-tarball/build.sh
```

The local CLP toolchain image built by `build.sh` includes `ldid`, which the
packaging step uses to apply ad-hoc Mach-O signatures after rewriting install
names. If you provide a prebuilt image, include `ldid` or set
`OSXCROSS_CODESIGN` to a compatible signing tool; otherwise the build fails
before packaging. For one-off local experiments, set
`OSXCROSS_ALLOW_UNSIGNED=true` to bypass this check and produce an unsigned
artifact. Do not release unsigned artifacts: they may fail at launch on macOS
with `Killed: 9`.

The dylib bundler fails if it cannot resolve a non-system Mach-O dependency.
This is intentional: every dependency outside `/usr/lib` and `/System` must be
found, copied into `lib/clp/`, and rewritten to load from the tarball. If this
check fails, install the missing dependency into the build dependency prefix,
fix the dependency's install name, or pass a colon-separated
`MACHO_DYLIB_SEARCH_ROOTS` value when invoking the lower-level packaging script.
Do not release a tarball with an unresolved non-system dependency; it may launch
on the build machine but fail on a clean macOS host.
`ldid` is AGPL-3.0 licensed. In this workflow it is used only as a build-time
tool inside the OSXCross image and is not copied into the generated CLP tarball.
If you distribute the build image itself, make sure that distribution complies
with `ldid`'s license terms.

The internal `osxcross-macports` dependency install currently uses a macOS 13.0
deployment target because the helper's binary-package mapping does not include
macOS 14.x; the final CLP configure/build still uses the deployment target above.
The image also exports `LD_LIBRARY_PATH=/opt/osxcross/target/lib` so OSXCross
linker tools can load bundled support libraries such as `libtapi`.

See `OSXCROSS_PLAN.md` for implementation notes and the validation checklist.

### Validation

The Docker build can inspect Mach-O metadata but cannot execute the produced
binaries on Linux. On a macOS host with the matching architecture, validate the
generated tarball with:

```shell
tmpdir="$(mktemp -d)"
tar -xzf packages/clp-core-*-macos-<arch>.tar.gz -C "${tmpdir}"
for b in clg clo clp clp-s indexer log-converter reducer-server; do
    "${tmpdir}"/clp-core-*/bin/"${b}" --help >/dev/null
done
otool -l "${tmpdir}"/clp-core-*/bin/clp | awk \
    '/LC_BUILD_VERSION/{show=1; count=0} show{print; count++} count==6{show=0}'
```

The `LC_BUILD_VERSION` output should show the configured minimum macOS version
as `minos` and the SDK version as `sdk`. To verify bundled dependency paths:

```shell
tmpdir="$(mktemp -d)"
tar -xzf packages/clp-core-*-macos-<arch>.tar.gz -C "${tmpdir}"
while IFS= read -r f; do
    otool -L "${f}" | rg '/opt/local|/opt/homebrew|/opt/osxcross|/clp/build|/Users'
done < <(find "${tmpdir}" -type f \( -perm -111 -o -name '*.dylib' \))
```

That command should print nothing.

### Native macOS build

Use `--native` when debugging with local macOS tooling or when running on a
macOS CI runner:

```shell
components/core/tools/packaging/macos-arm64-tarball/build.sh --native
```

Native builds default to the host architecture. Supplying `--arch` is supported,
but the value must match the host architecture because the local Homebrew
dependency set is architecture-specific.

Prerequisites:
- Xcode Command Line Tools (`xcode-select --install`)
- Homebrew deps:
  ```shell
  components/core/tools/scripts/lib_install/macos/install-all.sh
  ```
- pipx tools (cmake, task, uv):
  ```shell
  components/core/tools/scripts/lib_install/pipx-packages/install-all.sh
  ```

The native route uses the same shared build/package pipeline and the same
deployment target default as the Dockerized route.

## v1 scope and deferred items

This is the minimum-viable Mac packaging path. The following are intentionally
out of scope and are tracked as follow-up work:

- **CI integration** — the existing `clp-core-build-macos.yaml` workflow
  builds + tests but does not produce a tarball. Adding a packaging job
  that runs `build.sh --native` and uploads the result via
  `actions/upload-artifact` is the natural next step. Until that exists,
  the tarball is produced only by running `build.sh` manually.
- **SBOM sidecar** — the existing Linux SBOM pipeline (syft + manifest
  merger in `common/generate-sbom.{sh,py}`) does not yet have a macOS
  variant. Adding one needs a Homebrew cataloger pass and a different
  manifest-parser set; separate change.
- **Developer ID signing + notarization** — would require an Apple Developer
  Program membership, Developer ID credentials, and `notarytool` integration.
  The current Docker/OSXCross flow intentionally stops at local/internal
  artifacts; see "Signing model" above.
- **`.pkg` installer** — useful for managed Mac fleets that can't run
  ad-hoc tarballs. Builds on top of the signing/notarization work.
- **Homebrew tap** — `brew install y-scope/tap/clp-core` would smooth
  the install UX but needs a separate `homebrew-tap` repo.
- **GitHub Release attachment** — currently the tarball is only
  produced locally. Promoting to a tagged-release asset is trivial once
  a release workflow exists.
