#!/usr/bin/env python3
"""Verify that a CLP package matches its SBOM file evidence."""

import argparse
import hashlib
import importlib.util
import json
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path


def _load_generator_module():
    module_path = Path(__file__).with_name("generate-sbom.py")
    spec = importlib.util.spec_from_file_location("generate_sbom", str(module_path))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _sha256_file(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _property_values(component, name):
    return [
        prop.get("value")
        for prop in component.get("properties", [])
        if isinstance(prop, dict) and prop.get("name") == name
    ]


def _property_value(component, name):
    values = _property_values(component, name)
    return values[0] if values else None


def _component_hash(component):
    for item in component.get("hashes", []):
        if isinstance(item, dict) and item.get("alg") == "SHA-256":
            return item.get("content")
    return None


def _canonical_file_components(components, source_name):
    by_path = {}
    duplicates = []
    for component in components:
        if _property_value(component, "clp:source") != source_name:
            continue
        path = _property_value(component, "clp:path")
        if not path:
            duplicates.append("<missing clp:path>")
            continue
        entry = {
            "sha256": _component_hash(component),
            "size": _property_value(component, "clp:size"),
            "mode": _property_value(component, "clp:mode"),
            "needed": sorted(_property_values(component, "clp:elf-needed")),
            "needed_error": _property_value(component, "clp:elf-needed-error"),
        }
        if path in by_path:
            duplicates.append(path)
        by_path[path] = entry
    return by_path, duplicates


def _print_limited(label, values):
    print(f"{label}: {len(values)}", file=sys.stderr)
    for value in values[:20]:
        print(f"  {value}", file=sys.stderr)
    if len(values) > 20:
        print(f"  ... {len(values) - 20} more", file=sys.stderr)


def _compare(recorded, actual):
    recorded_paths = set(recorded)
    actual_paths = set(actual)
    missing = sorted(actual_paths - recorded_paths)
    extra = sorted(recorded_paths - actual_paths)
    changed = []
    for path in sorted(recorded_paths & actual_paths):
        if recorded[path] != actual[path]:
            changed.append(path)

    if not missing and not extra and not changed:
        return True

    print("ERROR: package contents do not match SBOM file evidence", file=sys.stderr)
    if missing:
        _print_limited("Files present in package but missing from SBOM", missing)
    if extra:
        _print_limited("Files present in SBOM but missing from package", extra)
    if changed:
        _print_limited("Files with mismatched evidence", changed)
        for path in changed[:5]:
            print(f"  {path}", file=sys.stderr)
            print(f"    SBOM:    {recorded[path]}", file=sys.stderr)
            print(f"    Package: {actual[path]}", file=sys.stderr)
    return False


def _upsert_metadata_property(sbom, name, value):
    metadata = sbom.setdefault("metadata", {})
    properties = metadata.setdefault("properties", [])
    properties[:] = [
        prop
        for prop in properties
        if not (isinstance(prop, dict) and prop.get("name") == name)
    ]
    properties.append({"name": name, "value": value})


def _run(argv, *, cwd=None, input_data=None, missing_tool=None):
    try:
        return subprocess.run(
            argv,
            check=True,
            cwd=cwd,
            input=input_data,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=False,
        )
    except FileNotFoundError:
        sys.exit(f"ERROR: {missing_tool or argv[0]} is required to verify package SBOMs")
    except subprocess.CalledProcessError as exc:
        stderr = (exc.stderr or b"").decode(errors="replace").strip()
        sys.exit(
            f"ERROR: {' '.join(argv)} failed with exit {exc.returncode}"
            + (f": {stderr}" if stderr else "")
        )


def _extract_deb(package_path, output_root, output_control):
    for argv in (
        ["dpkg-deb", "-x", str(package_path), str(output_root)],
        ["dpkg-deb", "-e", str(package_path), str(output_control)],
    ):
        _run(argv, missing_tool="dpkg-deb")


def _extract_rpm(package_path, output_root, _output_control):
    cpio_stream = _run(["rpm2cpio", str(package_path)], missing_tool="rpm2cpio").stdout
    _run(
        ["cpio", "-idmu", "--quiet"],
        cwd=output_root,
        input_data=cpio_stream,
        missing_tool="cpio",
    )


def _safe_extract_tar(package_path, output_root):
    output_root = Path(output_root).resolve()
    with tarfile.open(package_path, "r:*") as archive:
        members = []
        for member in archive.getmembers():
            target = (output_root / member.name).resolve()
            if not str(target).startswith(str(output_root) + "/") and target != output_root:
                sys.exit(f"ERROR: unsafe path in package archive: {member.name}")
            if member.issym() or member.islnk():
                link_target = Path(member.linkname)
                if link_target.is_absolute():
                    sys.exit(f"ERROR: unsafe absolute link in package archive: {member.name}")
                resolved_link = (target.parent / link_target).resolve()
                if (
                    not str(resolved_link).startswith(str(output_root) + "/")
                    and resolved_link != output_root
                ):
                    sys.exit(f"ERROR: unsafe link target in package archive: {member.name}")
            members.append(member)
        archive.extractall(str(output_root), members=members)


def _extract_apk(package_path, output_root, _output_control):
    try:
        _safe_extract_tar(package_path, output_root)
    except tarfile.TarError as exc:
        sys.exit(f"ERROR: failed to extract APK archive {package_path}: {exc}")


def _infer_format(package_path):
    suffix = package_path.suffix.lower()
    if suffix in (".deb", ".rpm", ".apk"):
        return suffix[1:]
    sys.exit(f"ERROR: cannot infer package format from extension: {package_path}")


def _extract_package(package_format, package_path, output_root, output_control):
    extractors = {
        "deb": _extract_deb,
        "rpm": _extract_rpm,
        "apk": _extract_apk,
    }
    extractors[package_format](package_path, output_root, output_control)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", required=True, help="Path to the built package.")
    parser.add_argument("--sbom", required=True, help="Path to the matching SBOM sidecar.")
    parser.add_argument(
        "--format",
        choices=["deb", "rpm", "apk"],
        default=None,
        help="Package format. Defaults to the package file extension.",
    )
    parser.add_argument(
        "--update-package-metadata",
        action="store_true",
        help="Stamp package format, filename, SHA-256, and size into metadata.properties.",
    )
    args = parser.parse_args()

    package_path = Path(args.package)
    sbom_path = Path(args.sbom)
    if not package_path.is_file():
        sys.exit(f"ERROR: package not found: {package_path}")
    if not sbom_path.is_file():
        sys.exit(f"ERROR: SBOM not found: {sbom_path}")
    package_format = args.format or _infer_format(package_path)

    try:
        sbom = json.loads(sbom_path.read_text())
    except json.JSONDecodeError as exc:
        sys.exit(f"ERROR: SBOM is not valid JSON ({sbom_path}): {exc}")

    generator = _load_generator_module()
    recorded, duplicates = _canonical_file_components(
        sbom.get("components", []),
        generator.PACKAGE_FILE_SOURCE,
    )
    if duplicates:
        _print_limited("Duplicate or malformed SBOM file evidence entries", duplicates)
        sys.exit(1)
    if not recorded:
        sys.exit("ERROR: SBOM contains no package file evidence")

    with tempfile.TemporaryDirectory(prefix="clp-sbom-verify-") as tmp:
        tmp_root = Path(tmp)
        package_root = tmp_root / "root"
        control_root = tmp_root / "DEBIAN"
        package_root.mkdir()
        control_root.mkdir()
        _extract_package(package_format, package_path, package_root, control_root)
        actual_components = generator.scan_package_files(
            package_root,
            control_root if package_format == "deb" else None,
        )

    actual, actual_duplicates = _canonical_file_components(
        actual_components,
        generator.PACKAGE_FILE_SOURCE,
    )
    if actual_duplicates:
        _print_limited("Duplicate generated package evidence entries", actual_duplicates)
        sys.exit(1)

    if not _compare(recorded, actual):
        sys.exit(1)

    if args.update_package_metadata:
        _upsert_metadata_property(sbom, "clp:package:format", package_format)
        _upsert_metadata_property(sbom, "clp:package:filename", package_path.name)
        _upsert_metadata_property(sbom, "clp:package:sha256", _sha256_file(package_path))
        _upsert_metadata_property(sbom, "clp:package:size", str(package_path.stat().st_size))
        sbom_path.write_text(json.dumps(sbom, indent=2) + "\n")

    print(f"==> Verified package/SBOM file evidence: {package_path.name}")


if __name__ == "__main__":
    main()
