#!/usr/bin/env python3
"""Generate a CycloneDX 1.5 SBOM for a CLP core package.

The output is a sidecar JSON file (``<pkg-basename>.sbom.cdx.json``) consumed
by Trivy as the security gate before package publication.

Inputs (all required):

* ``--syft-input``. A syft-produced CycloneDX document. syft is run with
  the rpm, dpkg, and apk package-database catalogers, capturing the
  OS-package surface (AlmaLinux 8 / Alpine) including development
  libraries that are statically linked into CLP binaries.
* ``--deps-yaml``. ``taskfiles/deps/main.yaml`` — the source of truth for
  the vendored C/C++ dependencies fetched from GitHub during
  ``task deps:core``. Each entry has a pinned version, a GitHub-derived
  PURL, and a tarball SHA-256.
* ``--source-install-script``. The family-specific
  ``install-packages-from-source.sh`` — pins liblzma, lz4, zstd, and
  libarchive versions as positional arguments to per-library installer
  scripts.
* ``--init-sh``. ``tools/scripts/deps-download/init.sh`` — pins the
  ``yscope-dev-utils`` commit used to populate the build-time taskfile
  submodule.
* ``--staging-dir`` and ``--staging-prefix``. The staged package payload
  used to produce file-level evidence for shipped binaries, bundled
  shared libraries, and package metadata such as ``DEBIAN/control``.

The merger composes these sources into a single CycloneDX document. See
``merge_components`` for the deduplication rules and ``components/core/tools/packaging/SBOM.md``
for the end-to-end design and audit checklist.

Exit codes: 0 on success, non-zero on any input failure (missing file,
unparseable YAML or JSON, empty merged component list).
"""

import argparse
import hashlib
import json
import re
import subprocess
import sys
import urllib.parse
from pathlib import Path

import yaml


SPEC_VERSION = "1.5"
PACKAGE_FILE_SOURCE = "package-staging"


class SbomManifestError(RuntimeError):
    """Raised when a manifest input has an unrecognizable structure.

    The merger does not tolerate silently-empty manifest contributions:
    syft's OS-package output would otherwise mask a regression in any of
    the manifest parsers and a package would ship with a partial SBOM
    that still passes downstream validation.
    """


# ---------------------------------------------------------------------------
# Vendored-dependency manifest parsing (taskfiles/deps/main.yaml)
# ---------------------------------------------------------------------------


_TEMPLATE_RE = re.compile(r"\{\{\s*\.([A-Za-z0-9_]+)\s*\}\}")


def _resolve_templates(value, scopes):
    """Resolve ``{{.VAR}}`` references in ``value`` against ``scopes``.

    Scopes are searched in order (innermost first). The substitution loop
    runs to a fixed point so chained references such as
    ``{{.URL_PREFIX}}/{{.VERSION}}`` resolve in a single call.
    """
    if isinstance(value, str):
        def repl(match):
            name = match.group(1)
            for scope in scopes:
                if name in scope:
                    return str(scope[name])
            return match.group(0)

        prev, cur = None, value
        while prev != cur:
            prev = cur
            cur = _TEMPLATE_RE.sub(repl, cur)
        return cur
    if isinstance(value, list):
        return [_resolve_templates(v, scopes) for v in value]
    if isinstance(value, dict):
        return {k: _resolve_templates(v, scopes) for k, v in value.items()}
    return value


def parse_deps_yaml(path):
    """Return one component per dependency built by ``task deps:core``.

    The dependency list is read from the ``core-all-parallel`` task's
    ``deps`` array; any task not listed there is skipped (so Spider-only
    dependencies such as ``mariadb-connector-cpp`` do not enter the SBOM).

    Raises ``SbomManifestError`` if the YAML structure has changed in a
    way that prevents extraction of the expected dependency list. This is
    a hard failure: a silently-empty manifest contribution would let
    syft's OS-package output mask the absence of CLP's most accurate
    upstream PURLs and SHA-256s.
    """
    data = yaml.safe_load(Path(path).read_text())
    if not isinstance(data, dict):
        raise SbomManifestError(
            f"{path}: top-level YAML is not a mapping (got {type(data).__name__})"
        )
    top_vars = data.get("vars") or {}
    tasks = data.get("tasks")
    if not isinstance(tasks, dict):
        raise SbomManifestError(f"{path}: missing or non-mapping 'tasks' key")

    core_task = tasks.get("core-all-parallel")
    if not isinstance(core_task, dict):
        raise SbomManifestError(
            f"{path}: missing 'tasks.core-all-parallel' anchor; the dependency "
            f"list cannot be derived"
        )
    core_dep_entries = core_task.get("deps") or []

    core_deps = []
    for entry in core_dep_entries:
        if isinstance(entry, dict) and "task" in entry:
            core_deps.append(entry["task"])
    if not core_deps:
        raise SbomManifestError(
            f"{path}: 'tasks.core-all-parallel.deps' resolved to zero task "
            f"entries; expected the vendored C/C++ dependency list"
        )

    components = []
    missing = []
    for dep_name in core_deps:
        task = tasks.get(dep_name)
        if not task:
            missing.append(dep_name)
            continue
        task_vars = task.get("vars") or {}
        scopes = [task_vars, top_vars]
        comp = _extract_dep(dep_name, task, scopes)
        if comp:
            components.append(comp)
        else:
            missing.append(dep_name)
    if missing:
        raise SbomManifestError(
            f"{path}: could not extract URL/SHA-256 for these tasks listed in "
            f"core-all-parallel: {missing}"
        )
    return components


def _extract_dep(name, task, scopes):
    """Extract a component from a task definition.

    A task may declare its tarball URL and SHA-256 in its own ``vars``
    block, in any ``cmds[].vars`` block, or in any ``deps[].vars`` block
    (this last form is used by tasks such as ``liblzma`` that delegate to
    a helper task). All three locations are inspected.
    """
    resolved = _resolve_templates(task, scopes)

    var_blocks = []
    if isinstance(resolved.get("vars"), dict):
        var_blocks.append(resolved["vars"])
    for section in ("cmds", "deps"):
        for entry in resolved.get(section) or []:
            if isinstance(entry, dict) and isinstance(entry.get("vars"), dict):
                var_blocks.append(entry["vars"])

    url, sha256 = None, None
    for block in var_blocks:
        if not url:
            for url_key in ("TARBALL_URL", "URL"):
                v = block.get(url_key)
                if isinstance(v, str):
                    url = v
                    break
        if not sha256:
            for sha_key in ("TARBALL_SHA256", "FILE_SHA256"):
                v = block.get(sha_key)
                if isinstance(v, str):
                    sha256 = v
                    break
        if url and sha256:
            break

    if not url:
        return None

    url = re.sub(r"\s+", "", url)
    version, purl = _derive_github_purl(url)
    properties = [
        {"name": "clp:source", "value": "taskfiles/deps/main.yaml"},
    ]
    # Mark heuristically-derived versions so auditors can distinguish them
    # from versions carried by a recognized PURL type. Today this only
    # applies to non-github.com URLs (e.g., antlr.org JAR, sqlite.org zip).
    if version and not purl:
        properties.append({"name": "clp:version-source", "value": "heuristic"})
    comp = {
        "type": "library",
        "bom-ref": f"clp-dep:{name}",
        "name": name,
        "version": version or "unknown",
        "scope": "required",
        "externalReferences": [{"type": "distribution", "url": url}],
        "properties": properties,
    }
    if purl:
        comp["purl"] = purl
    if sha256:
        comp["hashes"] = [{"alg": "SHA-256", "content": sha256.strip().lower()}]
    return comp


def _derive_github_purl(url):
    """Derive ``(version, purl)`` from a distribution URL.

    Returns ``(None, None)`` if the URL is not on github.com and no version
    can be extracted from its filename. Returns ``(version, None)`` for
    non-GitHub URLs whose filename contains a recognizable version literal.
    """
    parsed = urllib.parse.urlparse(url)
    if parsed.netloc != "github.com":
        fname = parsed.path.rsplit("/", 1)[-1]
        fname = re.sub(r"\.(tar\.gz|tgz|tar\.xz|zip|jar)$", "", fname)
        match = re.search(r"(\d+(?:\.\d+){0,3})", fname)
        return (match.group(1) if match else None), None

    parts = [p for p in parsed.path.split("/") if p]
    if len(parts) < 4:
        return None, None
    owner, repo = parts[0], parts[1]

    tag = None
    if parts[2] == "releases" and parts[3] == "download" and len(parts) >= 5:
        tag = parts[4]
    elif parts[2] == "archive":
        if len(parts) >= 6 and parts[3] == "refs" and parts[4] == "tags":
            tag = parts[5]
        elif len(parts) >= 4:
            tag = parts[3]
        if tag:
            tag = re.sub(r"\.(tar\.gz|tgz|zip)$", "", tag)
    if not tag:
        return None, None

    version = tag[1:] if tag.startswith("v") else tag
    purl = f"pkg:github/{owner}/{repo}@{tag}"
    return version, purl


# ---------------------------------------------------------------------------
# Source-built dependency parsing (install-packages-from-source.sh)
# ---------------------------------------------------------------------------


# Matches lines such as:  "${lib_install_scripts_dir}/lz4.sh" 1.10.0
_SOURCE_SCRIPT_RE = re.compile(
    r'"\$\{lib_install_scripts_dir\}/([a-zA-Z0-9_\-]+)\.sh"\s+(\S+)'
)


def parse_source_install_script(path):
    """Return one component per library built by install-packages-from-source.sh.

    Each library is installed into the builder image and may be either
    statically or dynamically linked into CLP binaries. The script does not
    expose a canonical upstream URL, so these components carry no PURL; the
    merge step relies on a name-and-version match to deduplicate them
    against the same upstream library when it also appears in
    ``taskfiles/deps/main.yaml``.

    Raises ``SbomManifestError`` if the regex matches zero invocations,
    which would indicate that the bash script's call convention changed
    and the parser is no longer recognizing any source-built library.
    """
    text = Path(path).read_text()
    components = []
    for match in _SOURCE_SCRIPT_RE.finditer(text):
        lib, version = match.group(1), match.group(2)
        components.append({
            "type": "library",
            "bom-ref": f"clp-source-built:{lib}",
            "name": lib,
            "version": version,
            "scope": "required",
            "description": (
                "Built from source by install-packages-from-source.sh. "
                "Linked into CLP binaries either statically or dynamically."
            ),
            "properties": [
                {"name": "clp:source", "value": "install-packages-from-source.sh"},
            ],
        })
    if not components:
        raise SbomManifestError(
            f"{path}: extracted zero source-built libraries; the bash call "
            f"convention may have changed (regex anchor: "
            f'"${{lib_install_scripts_dir}}/<name>.sh" <version>)'
        )
    return components


# ---------------------------------------------------------------------------
# Build-tool dependency parsing (tools/scripts/deps-download/init.sh)
# ---------------------------------------------------------------------------


_INIT_SH_SHA_RE = re.compile(
    r'YSCOPE_DEV_UTILS_COMMIT_SHA\s*=\s*"([0-9a-fA-F]{7,40})"'
)


def parse_init_sh(path):
    """Extract the yscope-dev-utils commit pinned in init.sh.

    yscope-dev-utils supplies the reusable taskfiles included by
    ``taskfiles/deps/main.yaml``; its code orchestrates the download and
    build of CLP's runtime dependencies but is not linked into any CLP
    binary. The component is emitted with ``scope: optional`` so that
    runtime CVE gates can filter it out, while ``clp:role=build-tool``
    flags it for supply-chain provenance audits.

    Raises ``SbomManifestError`` if the file is unreadable or if the
    pinned-SHA assignment cannot be located. The wrapper pre-validates
    that the file exists, so an unreadable file here indicates a permission
    or filesystem error worth surfacing rather than silently dropping the
    build-tool component.
    """
    text = Path(path).read_text()
    match = _INIT_SH_SHA_RE.search(text)
    if not match:
        raise SbomManifestError(
            f"{path}: could not locate YSCOPE_DEV_UTILS_COMMIT_SHA "
            f'assignment (expected pattern: YSCOPE_DEV_UTILS_COMMIT_SHA="<sha>")'
        )
    sha = match.group(1)
    return [{
        "type": "library",
        "bom-ref": f"clp-build-tool:yscope-dev-utils@{sha}",
        "name": "yscope-dev-utils",
        "version": sha,
        "scope": "optional",
        "purl": f"pkg:github/y-scope/yscope-dev-utils@{sha}",
        "description": (
            "Reusable taskfiles included by taskfiles/deps/main.yaml. "
            "Build-time only; not linked into CLP binaries."
        ),
        "externalReferences": [{
            "type": "vcs",
            "url": f"https://github.com/y-scope/yscope-dev-utils/tree/{sha}",
        }],
        "properties": [
            {"name": "clp:source", "value": "tools/scripts/deps-download/init.sh"},
            {"name": "clp:role",   "value": "build-tool"},
        ],
    }]


# ---------------------------------------------------------------------------
# Bundled shared-library discovery (bundle-libs.sh output)
# ---------------------------------------------------------------------------


# A bundled shared library matches ``libNAME.so`` optionally followed by
# one or more dot-separated numeric segments (the SONAME version suffix).
_BUNDLED_SO_RE = re.compile(r"^(?P<stem>lib.+?)\.so(?:\.(?P<version>[\d.]+))?$")


def scan_bundled_libs(staging_dir, staging_prefix):
    """Return one component per shared library staged for bundling.

    Inputs are the absolute paths used by ``bundle-libs.sh``; the directory
    walked is ``${staging_dir}${staging_prefix}/lib/clp``. Symlinks are
    skipped because their resolved targets are also present and would
    double-count. Filenames that do not match the ``libNAME.so[.X[.Y[.Z]]]``
    convention are skipped (defense in depth; ``bundle-libs.sh`` only
    stages real shared libraries here).
    """
    lib_dir = Path(staging_dir + staging_prefix) / "lib" / "clp"
    if not lib_dir.is_dir():
        return []
    install_path_root = f"{staging_prefix}/lib/clp"
    components = []
    for so in sorted(lib_dir.iterdir()):
        if not so.is_file() or so.is_symlink():
            continue
        match = _BUNDLED_SO_RE.match(so.name)
        if not match:
            continue
        name = match.group("stem")
        version = (match.group("version") or "unknown").rstrip(".")
        components.append({
            "type": "library",
            "bom-ref": f"clp-bundled:{so.name}",
            "name": name,
            "version": version or "unknown",
            "scope": "required",
            "description": f"Shared library bundled into the package at {install_path_root}/{so.name}.",
            "properties": [
                {"name": "clp:source",       "value": "bundle-libs.sh"},
                {"name": "clp:soname",       "value": so.name},
                {"name": "clp:install-path", "value": f"{install_path_root}/{so.name}"},
            ],
        })
    return components


# ---------------------------------------------------------------------------
# Package file evidence
# ---------------------------------------------------------------------------


_HELPER_FILES = frozenset({
    ".bundled-os-packages.txt",
    ".PKGINFO",
    ".INSTALL",
})
_NEEDED_RE = re.compile(r"Shared library: \[(?P<name>[^\]]+)\]")


def _is_helper_file(rel_path):
    return (
        len(rel_path.parts) == 1
        and (
            rel_path.name in _HELPER_FILES
            or rel_path.name.startswith(".SIGN.")
            or rel_path.name.startswith(".pre-")
            or rel_path.name.startswith(".post-")
        )
    )


def _sha256_file(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _is_elf(path):
    try:
        with Path(path).open("rb") as f:
            return f.read(4) == b"\x7fELF"
    except OSError:
        return False


def _readelf_needed(path):
    """Return ``(needed, error)`` for an ELF file.

    ``needed`` is sorted and deduplicated. ``error`` is a short diagnostic
    when ``readelf`` cannot be invoked or cannot parse the file. Non-ELF files
    return ``([], None)``.
    """
    if not _is_elf(path):
        return [], None
    try:
        out = subprocess.run(
            ["readelf", "-d", str(path)],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            timeout=10,
        )
    except FileNotFoundError:
        return [], "readelf not found"
    except subprocess.CalledProcessError as exc:
        err = (exc.stderr or exc.stdout or "").strip().splitlines()
        return [], f"readelf exited {exc.returncode}: {err[0] if err else 'no output'}"
    except subprocess.TimeoutExpired:
        return [], "readelf timed out"

    needed = sorted(set(_NEEDED_RE.findall(out.stdout or "")))
    return needed, None


def _file_component(path, package_path):
    stat_result = Path(path).stat()
    mode = stat_result.st_mode & 0o7777
    needed, needed_error = _readelf_needed(path)
    properties = [
        {"name": "clp:source", "value": PACKAGE_FILE_SOURCE},
        {"name": "clp:path", "value": package_path},
        {"name": "clp:size", "value": str(stat_result.st_size)},
        {"name": "clp:mode", "value": f"{mode:04o}"},
    ]
    properties.extend({"name": "clp:elf-needed", "value": lib} for lib in needed)
    if needed_error:
        properties.append({"name": "clp:elf-needed-error", "value": needed_error})

    return {
        "type": "file",
        "bom-ref": f"clp-file:{package_path}",
        "name": package_path,
        "scope": "optional",
        "hashes": [{"alg": "SHA-256", "content": _sha256_file(path)}],
        "properties": properties,
    }


def scan_package_files(package_root, control_dir=None):
    """Return file components for the package payload and control metadata.

    ``package_root`` is the root of the filesystem that will be installed by
    the package. ``control_dir`` may point at a Debian control directory; when
    present, regular files in it are recorded under ``DEBIAN/<name>``.
    """
    root = Path(package_root)
    components = []
    if root.is_dir():
        for path in sorted(p for p in root.rglob("*") if p.is_file()):
            rel = path.relative_to(root)
            if rel.parts and rel.parts[0] == "DEBIAN":
                continue
            if _is_helper_file(rel):
                continue
            components.append(_file_component(path, f"/{rel.as_posix()}"))

    if control_dir:
        control_root = Path(control_dir)
        if control_root.is_dir():
            for path in sorted(p for p in control_root.rglob("*") if p.is_file()):
                rel = path.relative_to(control_root)
                components.append(_file_component(path, f"DEBIAN/{rel.as_posix()}"))

    return components


# ---------------------------------------------------------------------------
# Build toolchain capture
# ---------------------------------------------------------------------------


_VERSION_RE = re.compile(r"(\d+(?:\.\d+){1,3})")


def _tool_version(argv):
    """Return ``(version_string, error)``.

    ``version_string`` is the first version literal found in the tool's
    output, or ``None`` if the tool could not be invoked or no version
    literal was found. ``error`` is a short diagnostic when the version
    is ``None``, suitable for emission as a CycloneDX ``properties`` entry.
    """
    try:
        # NOTE: AlmaLinux 8's system python3 is 3.6.8, which lacks the
        # `capture_output` shortcut (added in 3.7). Use the explicit
        # stdout/stderr PIPE form for compatibility with both manylinux
        # (3.6.8) and Alpine 3.20+ (3.12).
        out = subprocess.run(
            argv,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            timeout=10,
        )
    except FileNotFoundError:
        return None, "command not found"
    except subprocess.CalledProcessError as exc:
        return None, f"command exited {exc.returncode}"
    except subprocess.TimeoutExpired:
        return None, "command timed out"
    text = (out.stdout or out.stderr or "").splitlines()
    if not text:
        return None, "command produced no output"
    match = _VERSION_RE.search(text[0])
    if not match:
        return None, "no version literal in first output line"
    return match.group(1), None


def capture_toolchain():
    """Return CycloneDX ``metadata.tools`` entries for the compiler chain.

    A tool whose version cannot be captured is still emitted with
    ``version: "unknown"`` and a ``clp:capture-error`` property recording
    the reason, so an auditor can distinguish "tool was not used" from
    "tool was used but its version could not be read".
    """
    tools = []
    for name, argv in [
        ("gcc",      ["gcc", "--version"]),
        ("g++",      ["g++", "--version"]),
        ("cmake",    ["cmake", "--version"]),
        ("ld",       ["ld", "--version"]),
        ("readelf",  ["readelf", "--version"]),
        ("patchelf", ["patchelf", "--version"]),
    ]:
        version, error = _tool_version(argv)
        entry = {"vendor": "system", "name": name, "version": version or "unknown"}
        if error:
            entry["properties"] = [{"name": "clp:capture-error", "value": error}]
        tools.append(entry)
    return tools


# ---------------------------------------------------------------------------
# Component merge
# ---------------------------------------------------------------------------


def merge_components(*sources):
    """Concatenate component lists and deduplicate.

    Three-stage deduplication:

    1. Components carrying a ``purl`` are keyed on the PURL string. The
       first source that supplies a given PURL wins; later occurrences are
       dropped.
    2. Components without a ``purl`` are keyed on ``(lowercased name,
       version)``. A non-PURL entry is dropped when any PURL-bearing entry
       already covers the same ``(name, version)`` pair, and otherwise
       collapses against other non-PURL entries by the same key.
    3. The remaining list is checked for ``bom-ref`` uniqueness. CycloneDX
       1.5 requires every ``bom-ref`` to be unique within the document;
       a collision (which would indicate a generator emitting an
       unexpected reference string) raises ``SbomManifestError``.

    The merged list is finally sorted by ``bom-ref`` to produce a
    canonical component ordering. Without this, syft's filesystem-walk
    order can cause two builds of the same source tree to differ in
    component order, defeating the reproducibility property documented in
    ``SBOM.md`` §8.

    Callers must pass sources in priority order. Manifest-derived sources
    (which carry pinned upstream PURLs and tarball SHA-256s) belong before
    syft output and the bundled-library fallback so they win on
    conflict.
    """
    by_purl = {}
    nonpurl = []
    for lst in sources:
        for comp in lst or []:
            if not isinstance(comp, dict):
                continue
            purl = comp.get("purl")
            if purl:
                by_purl.setdefault(purl, comp)
            else:
                nonpurl.append(comp)

    covered_nv = {
        ((c.get("name") or "").lower(), c.get("version") or "")
        for c in by_purl.values()
    }
    seen_nv = set()
    filtered = []
    for comp in nonpurl:
        nv = ((comp.get("name") or "").lower(), comp.get("version") or "")
        if nv in covered_nv or nv in seen_nv:
            continue
        seen_nv.add(nv)
        filtered.append(comp)

    merged = list(by_purl.values()) + filtered

    seen_bom_refs = {}
    for comp in merged:
        ref = comp.get("bom-ref")
        if not ref:
            continue
        if ref in seen_bom_refs:
            raise SbomManifestError(
                f"bom-ref collision detected: '{ref}' is emitted by both "
                f"{seen_bom_refs[ref].get('name')!r} and {comp.get('name')!r}. "
                f"CycloneDX 1.5 requires every bom-ref to be unique within "
                f"the document."
            )
        seen_bom_refs[ref] = comp

    merged.sort(key=lambda c: (c.get("bom-ref") or "", c.get("name") or ""))
    return merged


# syft is invoked with an explicit list of OS package-database catalogers
# (rpm-db-cataloger, dpkg-db-cataloger, apk-db-cataloger), so its output
# contains only rpm, dpkg, or apk entries — no language-ecosystem noise,
# no SWID tags, no file-level ELF inventory. As a defense-in-depth check
# against a future cataloger-selection drift, the loader drops any
# component that arrives without a PURL, with one exception: a single
# ``type: operating-system`` component (e.g., ``alpine@3.22.4``,
# ``almalinux@8.10``) is preserved when present. That component has no
# PURL by CycloneDX convention but is what Trivy uses to select the
# OS-specific vulnerability feed; without it, Trivy reports
# ``Detected OS family="none"`` and skips OS-package scanning entirely.

# OS packages installed in the builder image that are NOT shipped in the
# CLP package — compilers, debuggers, linkers, build orchestration, text
# editors, package managers, scripting runtimes used only at build time,
# and kernel headers. Components matching this set are emitted with
# ``scope: optional`` so the downstream Trivy gate can filter them out of
# the runtime CVE surface while their presence in the build environment
# is still recorded for supply-chain auditing.
#
# Conservative policy: when in doubt, leave a component at the default
# ``scope: required``. A false positive (Trivy flagging a build-only CVE)
# is easier to handle downstream than a false negative (Trivy missing a
# runtime CVE because we silenced it here).
#
# Covers the union of AlmaLinux 8 (manylinux_2_28) and Alpine 3.22
# (musllinux_1_2) package names. A package name appearing on one
# distribution but not the other simply doesn't match on the other and
# is a no-op there.
_BUILD_ONLY_PKG_NAMES = frozenset({
    # ----- Kernel + bootloader (CLP runs in userspace, no kernel) -------
    "kernel-headers", "linux-headers",

    # ----- Compilers / linkers / debuggers / profilers ------------------
    "binutils", "gdb", "gdb-headless", "gcc-gdb-plugin",
    "valgrind", "valgrind-gdb", "valgrind-devel", "valgrind-docs",
    "valgrind-scripts",

    # ----- Build orchestration ------------------------------------------
    "make", "cmake-rpm-macros", "autoconf", "automake", "libtool",
    "m4", "patch", "bison", "flex", "pkgconf", "pkgconf-pkg-config",
    "pkgconf-m4", "libpkgconf",
    # RPM-build-time helpers (SRPM macros, debugging-info gen, etc.)
    "redhat-rpm-config", "dwz", "scl-utils", "annobin",
    "python-rpm-macros", "python-srpm-macros", "python3-rpm-macros",
    "efi-srpm-macros", "ghc-srpm-macros", "go-srpm-macros",
    "ocaml-srpm-macros", "openblas-srpm-macros", "qt5-srpm-macros",
    "rust-srpm-macros", "perl-srpm-macros",

    # ----- Text editors / pagers / build-time shell tools --------------
    "vim-minimal", "vim-common", "vim-enhanced", "less", "nano",
    "emacs-filesystem",

    # ----- VCS + transport used at build time --------------------------
    "git", "git-core", "git-core-doc", "openssh-clients",

    # ----- Package management runtimes themselves ----------------------
    "rpm", "rpm-libs", "rpm-build", "rpm-build-libs", "rpm-sign-libs",
    "rpm-plugin-systemd-inhibit",
    "dnf", "dnf-data", "dnf-plugins-core", "yum", "dpkg", "apk-tools",
    "libcomps", "libdnf", "libmodulemd", "libsolv", "librepo",
    # Python bindings used only by dnf/rpm itself
    "python3-dnf", "python3-dnf-plugins-core", "python3-rpm",
    "python3-hawkey", "python3-libcomps", "python3-libdnf",
    "python3-gpg",

    # ----- Documentation / man pages -----------------------------------
    "man-db", "info", "groff-base",

    # ----- Archive CLI tools (libs tracked separately) -----------------
    "tar", "gzip", "bzip2", "xz", "zip", "unzip", "cpio",

    # ----- Build-time Python ------------------------------------------
    "platform-python", "platform-python-pip", "platform-python-setuptools",
    "python3", "python3-libs", "python3-pip", "python3-pip-wheel",
    "python3-setuptools", "python3-setuptools-wheel",
    "python3.11", "python3.11-libs", "python3.11-pip",
    "python3.11-pip-wheel", "python3.11-setuptools",
    "python3.11-setuptools-wheel", "pipx",
    "python36", "python3-dateutil", "python3-six", "python3-pyyaml",
    # CLP doesn't run dbus/systemd/audit/selinux; their Python bindings
    # are pulled in by other packages but provide nothing to CLP.
    "python3-dbus", "python3-systemd", "python3-audit",
    "python3-libselinux", "python3-libsemanage", "python3-policycoreutils",
    "python3-setools",

    # ----- Build-time Perl --------------------------------------------
    "perl", "perl-libs", "perl-interpreter",

    # ----- Image / document / printing libraries (CLP processes neither
    #       images nor PDFs nor desktop documents) ----------------------
    "libtiff", "libtiff-devel", "libtiff-tools",
    "jasper", "jasper-libs",
    "libpng", "libpng-devel",
    "cups-libs", "cups-libs-devel",
    "jbigkit-libs", "lcms2", "libjpeg-turbo", "libxslt", "freetype",

    # ----- XML parsing (CLP processes JSON, not XML) ------------------
    "expat", "expat-devel", "libxml2", "libxml2-devel",

    # ----- HTTP/networking libraries unrelated to CLP -----------------
    # libcurl and openldap ARE bundled by bundle-libs.sh (curl LDAP
    # backend) and stay `required`. libsoup is a GNOME HTTP client CLP
    # never consumes; libproxy is a proxy-config helper not used at
    # runtime.
    "libsoup", "libproxy",

    # ----- Command-line JSON tool — used in build/diagnostic shells
    # only; the runtime JSON parsing in CLP uses the vendored simdjson
    # library.
    "jq",

    # ----- Alternate TLS / crypto stacks (CLP uses OpenSSL) -----------
    "nss", "nspr", "gnutls", "gnutls-utils",
    "nss-softokn", "nss-softokn-freebl", "nss-sysinit", "nss-util",
    # libgcrypt / libtasn1 are pulled in by gnutls and rpm. CLP does
    # not link or load either.
    "libgcrypt", "libtasn1",

    # ----- CLI wrappers around libs that are already tracked. The
    # runtime CVE surface lives on the library package
    # (openssl-libs, libcurl, libzstd, ncurses-libs); the CLI binary
    # itself never ships in CLP and would otherwise duplicate every CVE
    # under a second package name.
    "openssl", "curl", "zstd", "ncurses",

    # ----- Compiler frontends. Same logic as gcc-toolset-* prefix: the
    # codegen is statically linked into CLP binaries, the frontend
    # binaries themselves never ship.
    "gcc", "gcc-c++", "cpp", "gcc-plugin-annobin",

    # ----- -devel / -headers / -static packages. These are pure
    # header / pkgconfig / archive artifacts consumed at compile time
    # only; the corresponding runtime library packages are tracked
    # separately and remain `required`. Listing each one explicitly
    # (rather than a suffix rule) keeps the policy auditable.
    "openssl-devel", "libcurl-devel", "glib2-devel",
    "glibc-devel", "glibc-headers",
    "libstdc++-devel", "zlib-devel", "zlib-static", "pcre2-devel",

    # ----- GLib2 base library. CLP does not link or load GLib; it
    # gets pulled into the builder image by dnf/rpm/dconf transitively.
    "glib2",

    # ----- Kerberos / GSSAPI ------------------------------------------
    # NOTE: previously listed here as "not consumed by CLP", but
    # bundle-libs.sh actually patchelf-bundles libgssapi_krb5.so.2,
    # libk5crypto.so.3, libkrb5.so.3, and libkrb5support.so.0 into
    # /usr/lib/clp/ (libcurl on AlmaLinux 8 IS built with GSSAPI). Their
    # syft entries must stay `required` so the runtime gate catches
    # krb5 CVEs. The structural override in `load_syft` would force them
    # back to `required` even if listed here, but keeping them out of
    # the denylist makes the policy obvious to a code reader.

    # ----- SSH / SFTP transport ---------------------------------------
    # NOTE: previously listed here, but bundle-libs.sh bundles
    # libssh.so.4 (libcurl SFTP backend). Kept required.

    # ----- OpenLDAP ---------------------------------------------------
    # NOTE: previously listed, but bundle-libs.sh bundles libldap-*.so.2
    # and liblber-*.so.2 (libcurl LDAP backend). Kept required.

    # ----- SELinux (PAM-NSS-loaded via libcurl's ldap path) -----------
    # NOTE: previously listed, but bundle-libs.sh bundles libselinux.so.1
    # transitively. Kept required.

    # ----- GPG / PGP / TPM (CLP does not sign or verify) --------------
    "gpgme", "gnupg2", "libassuan", "libksba", "npth", "pinentry",
    "tpm2-tss", "trousers", "trousers-lib", "ima-evm-utils",
    # gpg-pubkey is an rpm-keyring entry, not real code.
    "gpg-pubkey",

    # ----- Auth / PAM / audit / cracklib (CLP runs unprivileged) ------
    "pam", "pam-devel", "audit", "audit-libs", "libpwquality",
    "cracklib", "cracklib-dicts", "shadow-utils", "libutempter",

    # ----- D-Bus / IPC plumbing (CLP does not use D-Bus) --------------
    "dbus", "dbus-libs", "dbus-common", "dbus-daemon", "dbus-tools",
    "dbus-glib", "dconf", "gsettings-desktop-schemas", "libgusb",
    "libmodman", "json-c", "json-glib",

    # ----- SELinux / LSM (containerized build, no policy enforcement)
    # libselinux itself is bundled via libcurl→libldap→libselinux (see
    # the carve-out comment above) and stays required. The other
    # SELinux userspace tools (libsemanage, libsepol, policy utilities,
    # libseccomp) are not bundled and are correctly build-only here.
    "libsemanage", "libsepol", "libsepol-devel",
    "libselinux-utils",
    "policycoreutils", "policycoreutils-python-utils", "checkpolicy",
    "libseccomp",

    # ----- Disk encryption / device-mapper / boot / init --------------
    "cryptsetup-libs", "device-mapper", "device-mapper-libs",
    "dracut", "kmod", "kmod-libs", "kbd", "kbd-legacy", "kbd-misc",
    "os-prober", "kpartx", "memstrack", "grubby",

    # ----- System service utilities not used by CLP -------------------
    "systemd", "libacl", "acl", "popt", "environment-modules",
    "hardlink", "hostname", "procps-ng", "util-linux",
    "libblkid", "libfdisk", "libmount", "libsmartcols", "libuuid",
    "uuid", "libcap", "libcap-ng",

    # ----- Networking debug / mDNS / SCTP / NIS -----------------------
    "iputils", "lksctp-tools", "libnsl2", "avahi-libs",

    # ----- mDNS / Bonjour --- already covered above ------------------

    # ----- Audio / multimedia -----------------------------------------
    "alsa-lib",

    # ----- Locale / timezone subpackages ------------------------------
    # tzdata stays required for runtime time-zone lookups by CLP.
    "glibc-langpack-en", "glibc-locale-source", "glibc-minimal-langpack",
    "tzdata-java", "langpacks-en",

    # ----- Other language runtimes never run by CLP -------------------
    "tcl", "tk", "guile", "lua", "lua-libs",

    # ----- Java JDK (CLP is C++) --------------------------------------
    "java-11-openjdk", "java-11-openjdk-headless",
    "copy-jdk-configs", "javapackages-filesystem",

    # ----- Distro metadata / release packages -------------------------
    "chkconfig", "setup", "epel-release", "almalinux-release",
    "basesystem", "filesystem", "rootfiles", "fontpackages-filesystem",

    # ----- Math / GCC dependencies (GCC links statically; the system
    #       copies are present but never loaded by CLP binaries) -------
    "isl", "gmp", "mpfr", "libmpc", "libatomic_ops",

    # ----- Build-time shell utilities CLP does not invoke -------------
    "gawk", "file", "file-libs", "which", "findutils",
    "coreutils-single", "diffutils", "grep", "sed",
    "elfutils", "elfutils-default-yama-scope",
    "elfutils-libelf", "elfutils-libs", "elfutils-debuginfod-client",

    # ----- Crypto-policies (system-wide TLS defaults; CLP applies its
    #       own OpenSSL config and ignores the policy) ----------------
    "crypto-policies", "crypto-policies-scripts",

    # ----- Network-config helpers (DNS resolver libs, public-suffix
    #       database). CLP statically links libidn2/libpsl/libnghttp2
    #       into clp-s — those packages remain required as separate
    #       components. The system copies of nghttp2/idn2 themselves
    #       are unused at CLP runtime; conservatively left required. --
    "publicsuffix-list-dafsa",

    # ----- Backup / TrueType / GUI / X11 single-package leftovers ----
    "fontconfig", "fribidi", "harfbuzz", "libthai", "libdatrie",
    "pixman", "pango",
    "atk", "at-spi2-atk", "at-spi2-core", "colord-libs",
    "abattis-cantarell-fonts", "ttmkfdir",
    "xkeyboard-config",
    "libfontenc", "libxshmfence", "libepoxy", "libusbx",
    "libxkbcommon",
    "gtk3", "gtk-update-icon-cache", "hicolor-icon-theme",
    "graphite2", "libICE", "libICE-devel", "libSM", "libSM-devel",
    "libxcrypt-devel", "keyutils-libs-devel",
    "rest", "glib-networking",

    # ----- Misc system-only libraries CLP does not consume -----------
    "oniguruma",          # regex used by jq (already optional) and ruby
    "cairo", "cairo-gobject",
    "ncurses-base",       # terminfo data (CLI binaries load via ncurses-libs)
    "libreport-filesystem",

    # =====================================================================
    # Alpine 3.22 (musllinux_1_2) names. The names below are LOWERCASE and
    # follow Alpine's package conventions, which differ from AlmaLinux's
    # PascalCase/-devel style — keep these as a separate block so an
    # auditor can verify each distro's coverage independently. The bundled
    # runtime libs on Alpine (musl, libssl3, libcrypto3, libcurl, libidn2,
    # libpsl, nghttp2-libs, brotli-libs, mariadb-connector-c, libarchive,
    # sqlite-libs, zlib, zstd-libs, xz-libs, bzip2-libs, lz4-libs, pcre2,
    # ncurses-libs, ncurses-terminfo-base, readline, libstdc++, libgcc,
    # libatomic, libgomp, bash, ca-certificates*, p11-kit*, tzdata,
    # libldap, libssh) must NOT appear in this block. See SBOM.md §11.5.
    # =====================================================================

    # ----- abuild / build toolchain -----------------------------------
    "abuild", "abuild-sudo", "alpine-sdk", "build-base", "fakeroot",
    "g++", "patchelf",
    "fortify-headers", "bsd-compat-headers", "musl-dev", "linux-pam",

    # ----- Clang / LLVM family -----------------------------------------
    "clang20-headers", "clang20-libs", "llvm20-libs",
    "spirv-llvm-translator-libs", "spirv-tools", "libclc", "isl26",

    # ----- Java JDK ----------------------------------------------------
    "openjdk11-jdk", "openjdk11-jmods", "openjdk11-jre",
    "openjdk11-jre-headless", "java-cacerts", "java-common",

    # ----- GPG / PGP family --------------------------------------------
    "gnupg", "gnupg-dirmngr", "gnupg-gpgconf", "gnupg-keyboxd",
    "gnupg-utils", "gnupg-wks-client", "gpg", "gpg-agent",
    "gpg-wks-server", "gpgsm", "gpgv", "pinentry",
    "libassuan", "libksba", "npth", "libgpg-error",

    # ----- DocBook / XML doc tooling -----------------------------------
    "docbook-xml", "docbook-xsl", "docbook-xsl-nons", "docbook-xsl-ns",
    "libxml2-utils",

    # ----- Python on Alpine -------------------------------------------
    # python3 itself is build-only; CLP does not ship a Python runtime.
    # The -pyc compiled bytecode packages are also build-only.
    "python3",
    "py3-packaging", "py3-packaging-pyc",
    "py3-parsing", "py3-parsing-pyc",
    "py3-yaml", "py3-yaml-pyc",
    "pyc", "python3-pyc", "python3-pycache-pyc0",

    # ----- busybox / util-linux family ---------------------------------
    "busybox", "busybox-binsh",
    "agetty", "cfdisk", "dmesg", "findmnt", "flock", "fstrim",
    "hexdump", "lscpu", "lsblk", "mcookie", "mount", "partx",
    "runuser", "setarch", "setpriv", "sfdisk", "shadow", "ssl_client",
    "umount", "uuidgen", "wipefs", "logger", "losetup", "blkid",
    "util-linux", "util-linux-misc",
    "libsmartcols",
    "libcap2", "libcap-getcap",

    # ----- Alpine distro metadata --------------------------------------
    "alpine-baselayout", "alpine-baselayout-data", "alpine-keys",
    "alpine-release", "apk-tools", "libapk2",
    "hwdata-pci", "scanelf", "skalibs-libs",

    # ----- GUI / X11 / Mesa / Wayland (Alpine lowercase names) --------
    "libx11", "libxau", "libxcb", "libxdamage", "libxdmcp",
    "libxext", "libxfixes", "libxft", "libxi", "libxrender",
    "libxshmfence", "libxtst", "libxxf86vm",
    "libice", "libpciaccess", "libsm",
    "mesa", "mesa-egl", "mesa-gbm", "mesa-gl", "mesa-gles",
    "mesa-rusticl", "mesa-xatracker",
    "wayland-libs-client", "wayland-libs-server",
    "xcb-proto", "xcb-proto-pyc", "xorgproto", "xtrans",

    # ----- Image / multimedia / font / i18n libs not used by CLP ------
    "alsa-lib", "freetype", "giflib", "lcms2",
    "libexpat", "libelf", "libgfortran",
    "nettle", "libpng", "libjpeg-turbo",
    "gmp", "mpc1", "mpdecimal", "mpfr4",
    "file", "libmagic", "diffutils",
    "gettext", "gettext-asprintf", "gettext-envsubst", "gettext-libs",
    "libintl", "acl-libs", "libbsd", "libmd", "libedit", "libffi",
    "libformw", "libmenuw", "libpanelw", "libncurses++",
    "yaml", "jansson", "glib",
})

# Name prefixes for entire build-only package families. ``gcc-toolset-``
# matches AlmaLinux 8's gcc-toolset stream (~25 packages — the C++17
# toolchain whose code is statically linked into CLP binaries, so the
# toolset packages themselves don't ship). ``libX``, ``mesa-``, etc.
# cover the X11/Wayland/OpenGL/GUI stack pulled in transitively by the
# manylinux base image but never reachable from a headless CLP binary.
_BUILD_ONLY_PKG_PREFIXES = (
    "gcc-toolset-",
    "gcc-gfortran", "gfortran", "libgfortran",
    "perl-",
    "python3.11-",
    "vim-",
    "grub2-",
    "systemd-",
    "gnome-",
    # GUI / desktop stack (AlmaLinux / RHEL — PascalCase X11 names)
    "libX",                # libX11, libXau, libXcursor, libXdamage, libXext, libXfixes, ...
    "libwayland-",         # libwayland-{client,cursor,egl,server}
    "libxcb",              # libxcb, libxcb-devel
    "libdrm",              # libdrm, libdrm-devel
    "libglvnd",            # libglvnd, libglvnd-{core-devel,devel,egl,gles,glx,opengl}
    "mesa-",               # mesa-libGL, mesa-libEGL, mesa-libgbm, ...
    "xorg-",               # xorg-x11-font-utils, xorg-x11-fonts-Type1, xorg-x11-proto-devel
    "adwaita-",            # cursor/icon themes
    "dejavu-",             # fonts
    "gdk-pixbuf",          # gdk-pixbuf2, gdk-pixbuf2-modules
)

# Name suffixes that mark a package as build-time-only by convention in
# both rpm and apk ecosystems. ``-devel`` / ``-headers`` ship only
# C headers + pkgconfig; ``-static`` ships only the .a archive consumed
# at link time; ``-debug`` / ``-debuginfo`` / ``-debugsource`` /
# ``-dbg`` / ``-doc`` / ``-pyc`` ship debug or documentation artifacts.
# A suffix rule collapses dozens of explicit names into one auditable
# pattern while staying limited to well-known build-only conventions.
# The runtime sibling (e.g., ``openssl-libs`` for ``openssl-devel``) is
# tracked separately and stays ``required`` unless explicitly listed.
_BUILD_ONLY_PKG_SUFFIXES = (
    "-devel",
    "-headers",
    "-static",
    "-dev",          # Alpine
    "-dbg",
    "-debug",
    "-debuginfo",
    "-debugsource",
    "-doc",
    "-pyc",          # Alpine compiled-Python bytecode
)


def _is_build_only(name):
    """Return True if ``name`` identifies a package that is build-only.

    A build-only package exists in the builder image (so syft inventoried
    it) but does not contribute to CLP's runtime CVE surface. The merger
    sets ``scope: optional`` on such components.

    Match order: explicit name → suffix → prefix. The suffix and prefix
    rules cover families that would otherwise require enumerating dozens
    of names; they are deliberately conservative and limited to
    well-known build-time conventions (`-devel`, `-headers`, `-static`,
    `-dev`, `-dbg`, `-debug`, `-pyc`, `-doc` suffixes and the GUI/X11/
    Mesa/Wayland prefixes).
    """
    if not name:
        return False
    if name in _BUILD_ONLY_PKG_NAMES:
        return True
    if any(name.endswith(s) for s in _BUILD_ONLY_PKG_SUFFIXES):
        return True
    return any(name.startswith(p) for p in _BUILD_ONLY_PKG_PREFIXES)


def _load_bundled_packages(path):
    """Return the set of OS-package names from ``.bundled-os-packages.txt``.

    The file is produced by ``bundle-libs.sh``: one OS-package name per line,
    sorted, deduped, corresponding to the .so files patchelf-bundled into
    the shipped package. Used as a structural override in ``load_syft``: any
    name in this set is forced to ``scope: required`` regardless of whether
    the build-only denylist would otherwise mark it optional.

    Returns an empty set if ``path`` is ``None`` or the file is absent
    (callers should treat that as "no structural override available" and
    rely on the denylist alone). Whitespace-only lines and blanks are
    skipped; everything else is taken verbatim.
    """
    if not path:
        return frozenset()
    p = Path(path)
    if not p.is_file():
        return frozenset()
    names = {line.strip() for line in p.read_text().splitlines() if line.strip()}
    return frozenset(names)


def load_syft(path, bundled_packages=frozenset()):
    """Load a syft-produced CycloneDX document and return ``(components, tools)``.

    Raises ``SbomManifestError`` if the file is missing, empty, unparseable,
    or contains fewer OS components than ``_MIN_SYFT_OS_COMPONENTS``. syft
    is the sole source of OS-package coverage in the merged SBOM, so a
    silently-empty result here would let a package ship with a partial
    SBOM that still passes downstream validation. The wrapper script also
    pre-validates that the file is non-empty; this loader is the second
    line of defense and adds the minimum-count check.

    Filter rules (see module-level comment):

    * Components with a PURL are kept.
    * Components of ``type: operating-system`` are kept even without a
      PURL — Trivy uses this entry to select the OS vulnerability feed.
    * All other no-PURL components are dropped and a count is logged.

    Scope tagging:

    * A name in ``bundled_packages`` (the structural override produced by
      ``bundle-libs.sh``) is forced to ``scope: required`` even if the
      denylist would tag it optional. Overrides are logged so a denylist
      drift surfaces in the build output instead of silently hiding CVEs.
    * Otherwise, a name in ``_BUILD_ONLY_PKG_NAMES`` /
      ``_BUILD_ONLY_PKG_PREFIXES`` is tagged ``scope: optional``.
    * Every other kept component is tagged ``scope: required`` explicitly
      so a downstream auditor grepping for ``"scope": "required"`` finds
      the full runtime CVE surface (CycloneDX's implicit default for
      absent scope is the same value, but tooling differs and the audit
      story is clearer when both halves of the policy are spelled out).

    The minimum-count check runs against PURL-bearing components only, so
    the threshold reflects OS-package coverage and is unaffected by the
    single OS marker component.
    """
    p = Path(path)
    if not p.is_file() or p.stat().st_size == 0:
        raise SbomManifestError(f"syft input missing or empty: {path}")
    try:
        data = json.loads(p.read_text())
    except json.JSONDecodeError as exc:
        raise SbomManifestError(f"syft input is not valid JSON ({path}): {exc}")
    raw_components = [c for c in (data.get("components") or []) if isinstance(c, dict)]
    pkg_components = [c for c in raw_components if c.get("purl")]
    os_components = [
        c for c in raw_components
        if not c.get("purl") and c.get("type") == "operating-system"
    ]

    build_only_count = 0
    override_count = 0
    overridden_names = []
    tagged_components = []
    for comp in pkg_components + os_components:
        name = comp.get("name") or ""
        comp = dict(comp)
        is_bundled = name in bundled_packages
        is_build_only = _is_build_only(name)
        if is_bundled:
            comp["scope"] = "required"
            if is_build_only:
                override_count += 1
                overridden_names.append(name)
        elif is_build_only:
            comp["scope"] = "optional"
            build_only_count += 1
        else:
            comp["scope"] = "required"
        tagged_components.append(comp)

    dropped = len(raw_components) - len(pkg_components) - len(os_components)
    if dropped:
        print(
            f"==> syft filter ({path}): kept {len(pkg_components)} package "
            f"+ {len(os_components)} OS marker components, dropped {dropped} "
            f"(no PURL, non-OS)",
            file=sys.stderr,
        )
    if build_only_count:
        print(
            f"==> syft scope tagging ({path}): {build_only_count} components "
            f"marked optional (build-only), "
            f"{len(tagged_components) - build_only_count} remain required",
            file=sys.stderr,
        )
    if override_count:
        # The denylist wanted these `optional` but bundle-libs.sh proves
        # they ship at runtime. Either remove them from the denylist or
        # accept the override; do not let them slip to `optional`.
        print(
            f"==> syft scope override ({path}): {override_count} components "
            f"forced to required (in bundled-packages manifest): "
            f"{', '.join(sorted(set(overridden_names)))}",
            file=sys.stderr,
        )
    if len(pkg_components) < _MIN_SYFT_OS_COMPONENTS:
        raise SbomManifestError(
            f"syft input produced {len(pkg_components)} PURL-bearing components, "
            f"expected at least {_MIN_SYFT_OS_COMPONENTS}; the scan may "
            f"have silently missed the OS package database"
        )

    tools = []
    meta_tools = (data.get("metadata") or {}).get("tools")
    if isinstance(meta_tools, list):
        for tool in meta_tools:
            if isinstance(tool, dict):
                tools.append(tool)
    elif isinstance(meta_tools, dict):
        for tool in meta_tools.get("components") or []:
            if isinstance(tool, dict):
                tools.append(tool)
    return tagged_components, tools


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


# Minimum-count assertions for the manifest parsers. These guard against
# silent regressions in the parsers themselves (e.g., the YAML structure
# changing in a way the existing extractor cannot follow): syft's
# OS-package output would otherwise mask the absence of CLP's most
# accurate upstream PURLs and SHA-256s. The values are sized to allow
# normal manifest growth/shrinkage without producing false alarms.
_MIN_MANIFEST_COMPONENTS = 20      # taskfiles/deps/main.yaml core-all-parallel
_MIN_SOURCE_BUILT_COMPONENTS = 4   # install-packages-from-source.sh
_REQUIRED_BUILD_TOOL_COMPONENTS = 1  # init.sh: exactly one pinned commit
# Lower bound on syft's OS-package count. Observed counts: ~480 on
# manylinux_2_28, ~250 on musllinux_1_2 (Alpine). The threshold is set
# well below both to tolerate base-image churn while still catching the
# "syft scan produced almost nothing" failure mode.
_MIN_SYFT_OS_COMPONENTS = 100


def build_sbom(args):
    manifest = parse_deps_yaml(args.deps_yaml)
    source_built = parse_source_install_script(args.source_install_script)
    build_tools = parse_init_sh(args.init_sh)
    bundled = scan_bundled_libs(args.staging_dir, args.staging_prefix)
    package_files = scan_package_files(
        args.staging_dir,
        Path(args.staging_dir) / "DEBIAN",
    )
    toolchain = capture_toolchain()
    bundled_packages = _load_bundled_packages(args.bundled_packages)
    syft_components, syft_tools = load_syft(args.syft_input, bundled_packages)

    if len(manifest) < _MIN_MANIFEST_COMPONENTS:
        raise SbomManifestError(
            f"vendored-dep manifest produced {len(manifest)} components, "
            f"expected at least {_MIN_MANIFEST_COMPONENTS}; the parser may "
            f"have silently lost dependencies"
        )
    if len(source_built) < _MIN_SOURCE_BUILT_COMPONENTS:
        raise SbomManifestError(
            f"source-built manifest produced {len(source_built)} components, "
            f"expected at least {_MIN_SOURCE_BUILT_COMPONENTS}"
        )
    if len(build_tools) != _REQUIRED_BUILD_TOOL_COMPONENTS:
        raise SbomManifestError(
            f"build-tool manifest produced {len(build_tools)} components, "
            f"expected exactly {_REQUIRED_BUILD_TOOL_COMPONENTS} "
            f"(the yscope-dev-utils pinned commit from init.sh)"
        )

    components = merge_components(
        manifest,
        source_built,
        build_tools,
        bundled,
        package_files,
        syft_components,
    )

    sbom = {
        "bomFormat": "CycloneDX",
        "specVersion": SPEC_VERSION,
        "version": 1,
        "metadata": {
            "tools": (
                [{"vendor": "y-scope", "name": "clp-sbom-generator", "version": "0.1"}]
                + toolchain
                + syft_tools
            ),
            "component": {
                "type": "application",
                "bom-ref": f"clp-core@{args.pkg_version}+{args.pkg_format}.{args.pkg_arch}",
                "name": args.pkg_name,
                "version": args.pkg_version,
                "purl": (
                    f"pkg:generic/{args.pkg_name}@{args.pkg_version}"
                    f"?arch={args.pkg_arch}&format={args.pkg_format}"
                ),
                "description": "CLP core universal binaries for log compression and search.",
                "supplier": {"name": "YScope Inc.", "url": ["https://yscope.com"]},
                "licenses": [{"license": {"id": "Apache-2.0"}}],
            },
            "properties": [
                {"name": "clp:deps-family", "value": args.deps_family},
                {"name": "clp:pkg-format",  "value": args.pkg_format},
                {"name": "clp:pkg-arch",    "value": args.pkg_arch},
            ],
        },
        "components": components,
    }
    return sbom


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--syft-input", required=True,
                        help="Path to the syft-produced CycloneDX JSON.")
    parser.add_argument("--bundled-packages", default=None,
                        help="Path to bundle-libs.sh's .bundled-os-packages.txt "
                             "(one OS-package name per line). Any name in this "
                             "file is forced to scope: required, overriding the "
                             "build-only denylist. Omit for denylist-only policy.")
    parser.add_argument("--deps-yaml", required=True)
    parser.add_argument("--source-install-script", required=True)
    parser.add_argument("--init-sh", required=True,
                        help="Path to tools/scripts/deps-download/init.sh "
                             "(source of the yscope-dev-utils pinned commit).")
    parser.add_argument("--staging-dir", required=True)
    parser.add_argument("--staging-prefix", required=True)
    parser.add_argument("--deps-family", required=True,
                        choices=["manylinux_2_28", "musllinux_1_2"])
    parser.add_argument("--pkg-name", required=True)
    parser.add_argument("--pkg-version", required=True)
    parser.add_argument("--pkg-arch", required=True)
    parser.add_argument("--pkg-format", required=True, choices=["deb", "rpm", "apk"])
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    try:
        sbom = build_sbom(args)
    except SbomManifestError as exc:
        sys.exit(f"ERROR: SBOM manifest input failed validation: {exc}")

    if sbom["bomFormat"] != "CycloneDX":
        sys.exit("ERROR: generated SBOM has wrong bomFormat")
    if sbom["specVersion"] != SPEC_VERSION:
        sys.exit("ERROR: generated SBOM has wrong specVersion")
    if not sbom["metadata"]["component"].get("name"):
        sys.exit("ERROR: generated SBOM has empty root component name")
    if not sbom["components"]:
        sys.exit("ERROR: generated SBOM has no components")

    Path(args.output).write_text(json.dumps(sbom, indent=2) + "\n")
    print(f"==> Wrote SBOM: {args.output}")
    print(f"    Components: {len(sbom['components'])}")


if __name__ == "__main__":
    main()
