# CLP Core Package SBOM

This document describes how CLP core's `.deb`, `.rpm`, and `.apk` packages
are accompanied by a CycloneDX 1.5 Software Bill of Materials (SBOM)
sidecar file, the design decisions behind the generation pipeline, and the
audit procedures used to validate its output.

## 1. Purpose

Every CLP core package produced by
[`build.sh`](./build.sh) is shipped together with a sidecar SBOM file:

```text
packages/clp-core_<ver>_<arch>.deb
packages/clp-core_<ver>_<arch>.deb.sbom.cdx.json
```

The SBOM is a CycloneDX 1.5 JSON document. Its primary consumer is
[Trivy], which scans the SBOM against vulnerability databases before the
package is promoted to a downstream repository (e.g., an internal APT
repository). A non-zero Trivy exit on HIGH or CRITICAL findings blocks
publication.

The sidecar name is the full package filename with `.sbom.cdx.json`
appended. The package format is therefore encoded in the sidecar's own
filename, and the package-to-sidecar relationship is a fixed-string
suffix that downstream automation can apply without parsing the
package basename:

| Format | Package filename                | Sidecar filename                             |
|--------|---------------------------------|----------------------------------------------|
| `.deb` | `clp-core_<ver>_<arch>.deb`     | `clp-core_<ver>_<arch>.deb.sbom.cdx.json`    |
| `.rpm` | `clp-core-<ver>-1.<arch>.rpm`   | `clp-core-<ver>-1.<arch>.rpm.sbom.cdx.json`  |
| `.apk` | `clp-core-<ver>-r0.apk`         | `clp-core-<ver>-r0.apk.sbom.cdx.json`        |

`<ver>` is the CMake project version with a build-metadata suffix —
e.g., `0.12.1~20260602.6bcecd82` on a CI build. The trailing `-1` (rpm)
and `-r0` (apk) are the package-format release numbers.

[Trivy]: https://github.com/aquasecurity/trivy

## 2. Scope

### In scope

* Every C/C++ library that contributes to a CLP binary's CVE surface,
  including:
  * vendored third-party libraries fetched from upstream tarballs;
  * libraries built from source inside the builder image;
  * dynamically-linked shared libraries bundled inside the package;
  * OS packages from the base image that are statically linked into a
    CLP binary (for example `openssl-devel`, `libcurl-devel`,
    `mariadb-connector-c-devel`).
* Build-time supply-chain dependencies whose code never ships in the
  binary but whose compromise would affect the build (for example
  `yscope-dev-utils`). These are emitted with `scope: optional` so that
  Trivy runtime-CVE gates can filter them out while preserving them for
  provenance audits.
* Build toolchain entries (`gcc`, `g++`, `cmake`, `ld`, `patchelf`)
  captured under `metadata.tools`.

### Out of scope

* Developer-environment tools (IDE, linters, formatters) that are not
  installed inside the builder image.
* Test-only dependencies that are not linked into the shipped binaries.
* Per-source-file authorship; the SBOM is at library granularity.

## 3. Dependency sources

CLP core resolves its C/C++ dependencies through six distinct sources.
Each source has a different on-disk format, a different parser, and a
distinct purpose. The table below is the audit reference: for every
component in a generated SBOM, the `properties[].clp:source` field
identifies which row of this table it came from.

| # | Source file | Format | Parser | PURL convention | `scope` |
|---|-------------|--------|--------|------------------|---------|
| 1 | [`taskfiles/deps/main.yaml`](../../../../taskfiles/deps/main.yaml) | YAML | `parse_deps_yaml` in `common/generate-sbom.py` | `pkg:github/<owner>/<repo>@<tag>` | `required` |
| 2 | [`components/core/tools/scripts/lib_install/<family>/install-prebuilt-packages.sh`](../scripts/lib_install/) | bash + dnf/apk | syft scan (see [§4.1](#41-syft-scan); the script in column 2 is the dependency declaration, not the parsed file) | `pkg:rpm/...` or `pkg:apk/...` | per [§6](#6-scope-tagging) |
| 3 | [`components/core/tools/scripts/lib_install/<family>/install-packages-from-source.sh`](../scripts/lib_install/) | bash | `parse_source_install_script` in `common/generate-sbom.py` | none (deduplicated by name+version against source #1) | `required` |
| 4 | [`components/core/tools/scripts/lib_install/pipx-packages/install-all.sh`](../scripts/lib_install/pipx-packages/) | bash + pipx | Not captured (see note below) | n/a | n/a |
| 5 | Base image (`quay.io/pypa/manylinux_2_28_*` or `quay.io/pypa/musllinux_1_2_*`) | OCI image | syft scan (see [§4.1](#41-syft-scan)) | `pkg:rpm/...` or `pkg:apk/...` | per [§6](#6-scope-tagging) |
| 6 | [`tools/scripts/deps-download/init.sh`](../../../../tools/scripts/deps-download/init.sh) | bash (regex on `YSCOPE_DEV_UTILS_COMMIT_SHA`) | `parse_init_sh` in `common/generate-sbom.py` | `pkg:github/y-scope/yscope-dev-utils@<sha>` | `optional` |

**Source 4 is intentionally absent from the merged SBOM.** The
pipx-installed tools (`cmake`, `go-task`, `uv`) are build-time only and
are not linked into any CLP binary, so they do not affect runtime CVE
exposure. The OS-only syft scan does not walk the pipx venvs. If
pipx-tool coverage becomes required, a second syft pass (with the
Python catalogers enabled over `/root/.local/share/pipx/venvs/`) can be
added to [`common/generate-sbom.sh`](./common/generate-sbom.sh).

### Why six sources, not one

CLP intentionally splits dependency declarations by lifecycle, not by
vendor:

* **Source 1** carries the vendored upstream libraries that CLP's CMake
  build expects to find. These are pinned by exact tarball URL + SHA-256
  in YAML so that `task deps:core` is reproducible.
* **Source 2** captures dynamic libraries provided by the host package
  manager. CLP does not bundle these; the runtime resolves them from the
  installed base image at the target host.
* **Source 3** rebuilds a small set of compression libraries from source
  because the versions shipped by AlmaLinux 8 and Alpine main are either
  outdated or lack the static archives that `libarchive` is statically
  linked against. The resulting `.a` files are bundled into other CLP
  binaries and therefore are real CVE surface.
* **Source 4** installs build-time Python tools (cmake, go-task, uv) into
  isolated pipx environments. These do not ship in any artifact.
* **Source 5** is the base image itself. The base image's package
  inventory is captured by walking the rpm or apk database.
* **Source 6** is the bootstrap submodule that supplies the taskfile
  utilities used by source 1. It does not ship in any artifact but is
  recorded for supply-chain provenance.

A single-manifest approach (for example a single `vcpkg.json` or
`conanfile.txt`) would not represent sources 2-6 accurately because
those sources do not have manifest semantics: source 2's versions are
host-resolved, source 5's are image-resolved, and so on. The hybrid
generation approach described below preserves each source's natural
semantics.

## 4. Generation pipeline

```text
build.sh (host)
  -> docker run --rm <builder-image> bash build-in-container.sh
       -> task deps:core   (downloads sources 1 + 6)
       -> task core        (compiles CLP)
       -> universal-<fmt>/package.sh
            -> common/bundle-libs.sh     (stages binaries + .so files;
                                          writes .bundled-os-packages.txt)
            -> common/generate-sbom.sh   (syft scan + Python merger;
                                          reads .bundled-os-packages.txt
                                          for the scope-required override)
            -> dpkg-deb / rpmbuild / abuild (packages binaries)
  -> copy <pkg> and <pkg>.sbom.cdx.json to ./packages/
```

The SBOM step runs **after** `bundle-libs.sh` (so that the bundled
shared libraries and their owning-OS-package manifest are present for
inspection) and **before** the format-specific build tool (so a build
that produces a package without a matching SBOM cannot occur).

### 4.1 syft scan

[`common/generate-sbom.sh`](./common/generate-sbom.sh) invokes syft once,
restricted to an explicit list of OS package-database catalogers:

```bash
syft scan dir:/ \
    --override-default-catalogers \
        "rpm-db-cataloger,dpkg-db-cataloger,apk-db-cataloger" \
    --output "cyclonedx-json@1.5=/tmp/clp-syft.cdx.json" \
    --quiet
```

This walks the rpm, dpkg, or apk package database under `/` inside the
builder container, producing CycloneDX 1.5 components with package-
manager PURLs (`pkg:rpm/...`, `pkg:apk/...`, `pkg:deb/...`). The
explicit cataloger list is intentionally narrower than syft's broader
`os` tag: the `os` tag also pulls in `binary-classifier-cataloger` and
`elf-binary-package-cataloger`, which would inventory every ELF in the
builder image and add tens of thousands of file-level entries with no
PURL. Those entries are not part of CLP's CVE surface; the bundled-
shared-library layer that *is* part of CLP is covered separately by
`scan_bundled_libs` in the merger.

The pass is mandatory: a non-zero exit, an empty output file, or an
output containing fewer than `_MIN_SYFT_OS_COMPONENTS` (currently 100)
PURL-bearing components aborts the build. syft is the sole source of
OS-package coverage in the merged SBOM, so silent partial output cannot
be allowed to pass the gate. `load_syft` drops any component arriving
without a PURL and logs the drop count as a defense-in-depth check
against future cataloger-selection drift. One exception: a single
`type: operating-system` component (e.g., `alpine@3.22.4`,
`almalinux@8.10`) is preserved when present, because Trivy uses that
component to select the OS-specific vulnerability feed — without it the
scan reports `Detected OS family="none"` and skips OS-package CVE
matching.

### 4.2 Python merger

[`common/generate-sbom.py`](./common/generate-sbom.py) reads five kinds
of input:

1. syft output (`--syft-input`);
2. `taskfiles/deps/main.yaml` (`--deps-yaml`);
3. `install-packages-from-source.sh` (`--source-install-script`);
4. `init.sh` (`--init-sh`);
5. `.bundled-os-packages.txt` (`--bundled-packages`, optional) — the
   set of OS packages that own the `.so` files bundled into the package
   by `bundle-libs.sh`. Used as the structural override described in
   [§6.2](#62-structural-override-for-bundled-libraries).

It also walks `${staging_dir}${prefix}/lib/clp` to record bundled `.so`
files and captures `gcc/g++/cmake/ld/patchelf` versions for
`metadata.tools`.

The merger then composes one CycloneDX document with the dedup rules
described in [§5](#5-merge-and-deduplication-rules) and the scope policy
in [§6](#6-scope-tagging).

## 5. Merge and deduplication rules

The merger calls `merge_components(*sources)` with the following source
order, which encodes a deliberate priority:

```python
merge_components(
    manifest,            # source 1 (YAML; pinned PURLs + SHA-256)
    source_built,        # source 3 (bash; no PURL)
    build_tools,         # source 6 (init.sh; pinned PURL)
    bundled,             # bundle-libs.sh staging walk; no PURL
    syft_components,     # syft scan (sources 2, 5; PURLs)
)
```

Deduplication runs in three stages:

1. **PURL stage.** Components carrying a `purl` are keyed on the PURL
   string. The first source to supply a given PURL wins; later
   occurrences are dropped. Manifest sources are listed first so that
   their accurate pinned PURLs (with tarball SHA-256s) survive over any
   syft-derived entry for the same logical component.
2. **Name-and-version stage.** Components without a `purl` are keyed on
   `(lowercased name, version)`. A non-PURL entry is dropped when any
   PURL-bearing entry already covers the same `(name, version)` pair,
   and otherwise collapses against other non-PURL entries by the same
   key. This is how the `lz4` entry from source 3 is collapsed into the
   manifest-derived `lz4` entry from source 1.
3. **`bom-ref` uniqueness stage.** A final pass verifies that no two
   merged components share a `bom-ref`. CycloneDX 1.5 requires
   `bom-ref` uniqueness within the document; a collision raises
   `SbomManifestError` and aborts the build.

The merged list is sorted by `bom-ref` to produce a canonical ordering
before emission. This is what guarantees the byte-stability property
documented in [§8](#8-reproducibility).

The merge does not attempt fuzzy name aliasing (for example, `zstd`
versus `zstandard`). When the same logical library appears under two
different names from different sources, both entries are emitted; the
PURL-bearing entry drives the CVE match in Trivy, and the non-PURL entry
provides additional traceability.

## 6. Scope tagging

CLP's downstream Trivy gate is scope-aware: a consumer filters the SBOM
on `scope != "optional"` before computing CVE counts. The merger's job
is to tag every shipped component so that the surviving set is the
truthful runtime CVE surface — and only that.

Three mechanisms cooperate inside `load_syft`:

* The **build-only denylist** ([§6.1](#61-build-only-denylist)) marks
  well-known build-only OS packages (compilers, debuggers, kernel
  headers, GUI stacks, documentation toolchains, `-dev`/`-devel`/
  `-static`/`-pyc` artifacts) as `scope: optional`.
* The **structural override** ([§6.2](#62-structural-override-for-bundled-libraries))
  forces `scope: required` on any OS package whose `.so` file is
  actually bundled into `/usr/lib/clp/`, regardless of denylist hits.
  This is the load-bearing safety net against denylist drift.
* The **explicit-required default** assigns `scope: required` to every
  remaining OS-package component. No syft component reaches the output
  with an unset `scope` field, so a downstream auditor grepping the
  SBOM for `"scope": "required"` finds the full runtime CVE surface
  without depending on CycloneDX's implicit default.

The merger logs per-batch counts to stderr during the build:

```text
==> syft scope tagging (/tmp/clp-syft.cdx.json): N components marked
optional (build-only), M remain required
```

### 6.1 Build-only denylist

The denylist lives in `_BUILD_ONLY_PKG_NAMES`,
`_BUILD_ONLY_PKG_PREFIXES`, and `_BUILD_ONLY_PKG_SUFFIXES` in
[`common/generate-sbom.py`](./common/generate-sbom.py). It covers the
union of AlmaLinux 8 and Alpine 3.22 package names whose code is
demonstrably build-only: never linked into a CLP binary, never bundled
into `/usr/lib/clp/`, and never invoked at runtime. Categories include
the gcc-toolset compiler stream, kernel headers, the X11/Wayland/Mesa
desktop stack, Java runtimes, GnuPG, busybox utilities, and the
`-dev`/`-devel`/`-static`/`-pyc`/`-doc`/`-dbg`/`-debuginfo` build-time
suffix families.

The policy is conservative: when a package is ambiguous (could be
runtime or build), it is left at `scope: required`. A Trivy false
positive on a build-only CVE is easier to handle than a runtime CVE
that gets silenced. Without the denylist applied, an unfiltered Trivy
scan of the `.deb`/`.rpm` sidecar reports thousands of CVEs against
build-only packages that never ship; with it, the required-scope CVE
surface reflects only the legitimately runtime libraries (the
static-linked deps and the bundled `.so` files).

### 6.2 Structural override for bundled libraries

A denylist that is mostly correct can still be wrong on a specific
package, and the cost of being wrong is high: a falsely-`optional`
runtime library hides its CVEs from the gate. The structural override
makes that class of bug impossible to ship.

`bundle-libs.sh` walks `ldd` over each compiled CLP binary, copies the
non-system shared libraries into `/usr/lib/clp/`, and then maps each
bundled `.so` back to its owning OS package using the package manager
available in the builder image: `rpm -qf` on manylinux, `apk info
--who-owns` on musllinux, `dpkg -S` as a fallback. The deduped set of
package names lands in `${DESTDIR}/.bundled-os-packages.txt`, which
`generate-sbom.sh` passes to the merger via `--bundled-packages`.
`load_syft` then forces `scope: required` on any syft component whose
name appears in that set, regardless of denylist hits.

When the override fires, the merger logs each affected name:

```text
==> syft scope override (/tmp/clp-syft.cdx.json): N components forced
to required (in bundled-packages manifest): libssh, krb5-libs, openldap, ...
```

A non-empty override line is a regression signal: the denylist has
drifted ahead of the bundling reality. The override silenced the
immediate bug, but the underlying fix is to remove each offending name
from `_BUILD_ONLY_PKG_NAMES` so the policy and the code agree. Treat
each override-line entry as a code-review item.

The authoritative source of truth for "what ships at runtime" is the
bundled manifest, not the denylist. If a future build adds, removes, or
relinks a `.so`, the override automatically picks up the new ground
truth from the next `bundle-libs.sh` run.

### 6.3 Downstream filtering

Most scanners do not yet honour CycloneDX `scope` natively. The cleanest
integration is to strip optional-scope components from the SBOM before
handing it to the scanner:

```bash
SBOM=packages/clp-core_<ver>_<arch>.deb.sbom.cdx.json
jq '.components |= map(select(.scope != "optional"))' "${SBOM}" > /tmp/sbom-required.json
trivy sbom /tmp/sbom-required.json --distro redhat/8
```

For a scope-aware post-process on Trivy JSON output (when stripping is
not desirable, e.g., for a single-pass scan that also wants to count
build-only findings separately):

```bash
trivy sbom "${SBOM}" --distro redhat/8 --format json \
  | jq --argjson opt "$(jq '[.components[]|select(.scope=="optional")|.name]' "${SBOM}")" '
      .Results[].Vulnerabilities[]
      | select(.PkgName as $n | $opt | index($n) | not)
    '
```

## 7. Failure model

Every failure in the SBOM generation pipeline is a hard build failure.
A package that ships without a corresponding SBOM would bypass the
downstream gate, so the build aborts at the first opportunity rather
than silently producing an unscannable artifact. Two narrow
soft-failure carve-outs are documented in [§7.2](#72-soft-failure-carve-outs).

### 7.1 Hard failures

| Failure | Detected at | Behavior |
|---------|-------------|----------|
| Missing required env var (`PKG_BASENAME`, etc.) | `common/generate-sbom.sh` | Exit 1 before the syft scan. |
| Missing manifest input (`deps/main.yaml`, `install-packages-from-source.sh`, `init.sh`) | `common/generate-sbom.sh` | Exit 1 before the syft scan. |
| `syft scan` non-zero exit or empty output file | `common/generate-sbom.sh` | Exit 1. |
| syft output is valid JSON but contains fewer than `_MIN_SYFT_OS_COMPONENTS` components | `load_syft` in `common/generate-sbom.py` raises `SbomManifestError` | `main()` catches and `sys.exit(...)`. Guards against the failure mode of an OS scanner silently producing an empty inventory. |
| Manifest parser cannot find its expected anchor (e.g., `core-all-parallel` removed from YAML, source-built script call convention changed, `YSCOPE_DEV_UTILS_COMMIT_SHA` removed from init.sh) | `common/generate-sbom.py` parsers raise `SbomManifestError` | `main()` catches and `sys.exit(...)` with the parser's diagnostic. |
| Manifest parser yields fewer components than the per-source minimum (vendored < 20, source-built < 4, build-tool != 1) | `build_sbom()` raises `SbomManifestError` | `main()` catches and `sys.exit(...)`. Guards against silent regressions that the parsers themselves cannot detect. |
| `bom-ref` collision between any two merged components | `merge_components()` raises `SbomManifestError` | `main()` catches and `sys.exit(...)`. CycloneDX 1.5 requires `bom-ref` uniqueness within the document. |
| Sidecar missing from `${repo_root}/build/` at copy time | `build.sh` post-build loop | Exit 1; `.deb`/`.rpm`/`.apk` not copied to output. |
| `init.sh` non-zero exit (cannot populate `tools/yscope-dev-utils`) | `build.sh` prerequisite step | Exit 1 via `set -o errexit`. |

### 7.2 Soft-failure carve-outs

Two intentional non-failures are logged but do not abort the build:

* **`bundle-libs.sh` cannot resolve a bundled `.so` to an OS package.**
  The resolver (`rpm -qf` / `apk info --who-owns` / `dpkg -S`) skips
  any path it cannot attribute and logs `WARNING: could not resolve
  owning package for <path>`. Failing the build here would block
  packaging on a single mis-installed library; the SBOM still ships
  with the structural-override set populated from the resolvable
  entries.
* **`.bundled-os-packages.txt` is missing or empty when the merger
  runs.** `load_syft` falls back to denylist-only policy. The merger
  emits no override line in this case, but the scope tagging itself
  still runs. A regression in `bundle-libs.sh` would surface as a
  missing override-log line in the build output, not a hard failure —
  catch it in the [§9 verification recipes](#9-verification-recipes)
  rather than at build time.

## 8. Reproducibility

The SBOM is intended to be deterministic across rebuilds of the same
source tree on the same builder image. The following are guaranteed
stable:

* The list of `components`, their PURLs, their SHA-256 hashes, and their
  ordering. These are derived from pinned manifest inputs (sources 1, 3,
  6) and from the builder image's package database (sources 2, 5), which
  itself is fixed by the base-image tag. The merger sorts the final
  `components` array by `bom-ref` so two builds of the same source tree
  on the same builder image produce a byte-identical components list.
* The merge order and deduplication rules.

The following are **not** guaranteed stable and may differ between
builds:

* `metadata.tools` version strings, if the builder image's compiler or
  syft has been updated since the last build.
* Any timestamps that syft may include in `metadata.timestamp`.

Reproducibility is verified by the procedure in
[§9.6](#96-reproducibility-check).

## 9. Verification recipes

The recipes below operate on a generated SBOM at
`packages/clp-core_<ver>_<arch>.deb.sbom.cdx.json`. Substitute the actual
filename when running them.

### 9.1 Well-formed CycloneDX 1.5

```bash
jq -r '.bomFormat, .specVersion, .metadata.component.name' \
    packages/clp-core_<ver>_<arch>.deb.sbom.cdx.json
# Expected:
#   CycloneDX
#   1.5
#   clp-core
```

### 9.2 Manifest dependencies have GitHub PURLs

```bash
jq '[.components[] | select((.purl // "") | startswith("pkg:github/"))] | length' \
    packages/clp-core_<ver>_<arch>.deb.sbom.cdx.json
# Expected: >= 21 (the deps in core-all-parallel + yscope-dev-utils)
```

Spot-check a known dependency:

```bash
jq '.components[] | select(.name == "absl")' \
    packages/clp-core_<ver>_<arch>.deb.sbom.cdx.json
# Expected: a component with version, purl, hashes[].alg == "SHA-256"
```

### 9.3 OS packages are present

```bash
jq '[.components[] | select((.purl // "") | startswith("pkg:rpm/"))] | length' \
    packages/clp-core_<ver>_<arch>.deb.sbom.cdx.json
# Expected: many (AlmaLinux 8 base + dev libs from source 2)
```

For an `.apk` SBOM, substitute `pkg:apk/`.

Spot-check `openssl-devel` (statically linked into CLP binaries):

```bash
jq '.components[] | select(.name | test("(?i)openssl"))' \
    packages/clp-core_<ver>_<arch>.deb.sbom.cdx.json
```

### 9.4 Scope tagging is sound

Every OS-package component must carry an explicit `scope`:

```bash
jq '[.components[] | select((.purl // "") | test("^pkg:(rpm|apk|deb)/")) | .scope]
    | group_by(.) | map({(.[0]): length}) | add' \
    packages/clp-core_<ver>_<arch>.deb.sbom.cdx.json
# Expected:  {"optional": N, "required": M}    (no missing scope)
```

The build log should NOT contain an `==> syft scope override` line. If
it does, the denylist has falsely marked a bundled package `optional`
and only the structural override
([§6.2](#62-structural-override-for-bundled-libraries)) prevented the
CVE silencing. Fix by removing each listed name from
`_BUILD_ONLY_PKG_NAMES` so the policy and the bundling reality agree.

### 9.5 Trivy can parse and scan the SBOM

```bash
trivy sbom packages/clp-core_<ver>_<arch>.deb.sbom.cdx.json --distro redhat/8
```

A non-zero exit code or a parser error is a failure. A scan that lists
components and reports zero or more CVEs is the expected outcome; the
HIGH/CRITICAL threshold for blocking publication is configured at the
gate, not in this document. `.apk` sidecars omit the `--distro` flag
(Alpine is a natively-supported Trivy OS family); see
[§12.3](#123-trivy-almalinux-compatibility-flag-for-debrpm-sidecars)
for the AlmaLinux compatibility background.

### 9.6 Reproducibility check

```bash
./components/core/tools/packaging/build.sh --format deb --arch x86_64 --clean
cp packages/clp-core_<ver>_amd64.deb.sbom.cdx.json /tmp/sbom-run1.json

./components/core/tools/packaging/build.sh --format deb --arch x86_64 --clean
diff <(jq -S '.components' /tmp/sbom-run1.json) \
     <(jq -S '.components' packages/clp-core_<ver>_amd64.deb.sbom.cdx.json)
```

The `components` array must be identical between runs. Differences in
`metadata.tools` or `metadata.timestamp` are expected.

## 10. Updating the syft pin

The syft version is pinned in exactly two places (kept in sync):

* [`universal-deb/Dockerfile`](./universal-deb/Dockerfile): `ARG SYFT_VERSION=...`
* [`alpine-apk/Dockerfile`](./alpine-apk/Dockerfile): `ARG SYFT_VERSION=...`

The install URL inside both Dockerfiles is parametric on `SYFT_VERSION`,
so a single ARG bump propagates to the install script too.

To update:

1. Pick the new syft release from
   [github.com/anchore/syft/releases](https://github.com/anchore/syft/releases).
2. Update both `ARG` declarations to the same value (including the
   leading `v`, e.g., `v1.45.0`).
3. Rebuild the two builder images:
   ```bash
   ./components/core/tools/packaging/build.sh --format all --arch <host-arch> --clean
   ```
4. Re-run the verification recipes in [§9](#9-verification-recipes).
5. Confirm that the component counts in the resulting SBOMs are
   unchanged or differ only as expected from syft's release notes.

## 11. Cross-references

| File | Purpose |
|------|---------|
| [`build.sh`](./build.sh) | Host-side entrypoint; invokes `init.sh`, builds Docker images, runs the in-container build, copies packages and sidecars to `./packages/`. |
| [`common/build-in-container.sh`](./common/build-in-container.sh) | In-container entrypoint; runs `task deps:core`, `task core`, then the format-specific `package.sh`. |
| [`common/bundle-libs.sh`](./common/bundle-libs.sh) | Stages compiled binaries and their shared-library dependencies into the packaging staging directory; rewrites RPATHs with patchelf; writes `${DESTDIR}/.bundled-os-packages.txt` (the set of OS packages that own the bundled `.so` files; consumed by `generate-sbom.py` as the [§6.2 structural override](#62-structural-override-for-bundled-libraries)). |
| [`common/generate-sbom.sh`](./common/generate-sbom.sh) | Runs the syft scan and invokes the Python merger. |
| [`common/generate-sbom.py`](./common/generate-sbom.py) | Parses the manifest sources and merges them with syft output into the final CycloneDX 1.5 document. |
| [`universal-deb/`](./universal-deb/) | Format-specific Dockerfile + `package.sh` for `.deb` packaging. |
| [`universal-rpm/`](./universal-rpm/) | Format-specific `package.sh` for `.rpm` packaging (shares the universal-deb Dockerfile). |
| [`alpine-apk/`](./alpine-apk/) | Format-specific Dockerfile + `package.sh` for `.apk` packaging. |
| [`taskfiles/deps/main.yaml`](../../../../taskfiles/deps/main.yaml) | Source 1 — vendored C/C++ dependency manifest. |
| [`tools/scripts/lib_install/`](../scripts/lib_install/) | Sources 2-4 — OS package installation, source-built libraries, pipx tools. |
| [`tools/scripts/deps-download/init.sh`](../../../../tools/scripts/deps-download/init.sh) | Source 6 — `yscope-dev-utils` bootstrap. |

## 12. Known limitations

### 12.1 GitHub PURL CVE coverage is partial

Components originating from source 1 (`taskfiles/deps/main.yaml`) and
source 6 (`init.sh`) carry GitHub-typed PURLs of the form
`pkg:github/<owner>/<repo>@<tag>`, where `<tag>` is the upstream git
tag (`v1.87.0`, `boost-1.87.0`, `cpp-7.0.0`, `r4.1.1`, etc., depending
on upstream's tagging convention).

Trivy's matching against these PURLs is feed-dependent. Many CVE feeds
key on language-ecosystem PURL types (`pkg:conan/...`, `pkg:generic/...`,
`pkg:pypi/...`) or on tag formats normalized to bare semver, not on
GitHub-typed PURLs with the upstream tag preserved verbatim. As a
result, Trivy may report zero findings for a manifest-derived component
even when the CVE feed has an entry, simply because the PURL type or
tag format did not match the feed's keying.

Pragmatically:

* OS-package PURLs (`pkg:rpm/...`, `pkg:apk/...`) from sources 2 and 5
  are matched reliably by Trivy.
* Manifest-derived PURLs from sources 1 and 6 are matched on a
  best-effort basis. Treat a clean Trivy report on these components as
  presumptive rather than authoritative.

This limitation is inherent to the choice of GitHub-typed PURLs and the
state of CVE-feed conventions; resolving it would require either a
multi-PURL emission strategy (one component carrying multiple PURL
candidates) or a normalization layer between the SBOM and the scanner.
Neither is in scope for the current pipeline. The `clp:source` property
on each component identifies its origin source so an auditor can
manually cross-reference any uncovered library against external CVE
databases.

### 12.2 Heuristic version extraction for non-GitHub URLs

For source-1 components whose distribution URL is not on `github.com`
(currently `antlr-jar` from `antlr.org` and `sqlite3` from
`sqlite.org`), the upstream version is extracted by regex from the
filename. These components carry a `clp:version-source=heuristic`
property in the SBOM so an auditor can spot heuristically-derived
versions and verify them against the upstream release.

### 12.3 Trivy AlmaLinux compatibility flag for `.deb`/`.rpm` sidecars

The deb and rpm sidecars are produced by walking the AlmaLinux 8
package database inside the manylinux_2_28 builder image, so their
OS-package PURLs use the `pkg:rpm/almalinux/<name>@<ver>` namespace and
the SBOM's top-level `components[]` entry of type
`operating-system` reports `name: almalinux, version: 8.10` (with a
`syft:distro:idLike=rhel` property). This is the accurate distro
identifier and preserves the link back to the actual builder.

Trivy versions through 0.71 do not yet recognize `almalinux` as a
first-class OS family in SBOM mode, even though AlmaLinux is binary-
compatible with RHEL. A scan with no extra flags emits
`Unsupported os family="almalinux"` and skips OS-package CVE matching.
The workaround is the `--distro` override, which tells Trivy to treat
the SBOM as RHEL 8 and match the `pkg:rpm/almalinux/*` entries against
the RHEL vulnerability feed:

```bash
trivy sbom <sidecar>.deb.sbom.cdx.json --distro redhat/8
trivy sbom <sidecar>.rpm.sbom.cdx.json --distro redhat/8
```

With the flag, Trivy detects CVEs against the RHEL 8 feed as expected
(for example, kernel, libpng, and openssl-libs CVEs surface and are
reported with the AlmaLinux package name and version). The `.apk`
sidecar does not need this flag — Alpine is a natively-supported
Trivy OS family.

If Trivy upstream adds first-class AlmaLinux support (already present
for AlmaLinux 9 in some Trivy releases), this section can be removed
without changing the SBOM. The data is unaffected; only the scanner
invocation differs.
