#!/usr/bin/env python3
"""Packages the CLP core executables built on a Debian-compatible system."""

from __future__ import annotations

import argparse
import logging
import shutil
import subprocess
import tempfile
from pathlib import Path

logger = logging.getLogger(__name__)
_DPKG = "dpkg"
_DPKG_DEB = "dpkg-deb"
_DPKG_SHLIBDEPS = "dpkg-shlibdeps"
_REQUIRED_BINARIES = ("clg", "clo", "clp", "clp-s", "indexer", "log-converter", "reducer-server")
_OPTIONAL_BINARIES = ("glt", "make-dictionaries-readable")


def create_debian_package(args: argparse.Namespace) -> Path:
    """
    Packages the standard core build, including legacy tools when they were built explicitly.

    Runtime dependencies are determined from the binaries using the build system's package
    database. Build and package on the Debian release where the package will be installed.

    :param args: Build directory, version, revision, and output directory.
    :return: The generated package path.
    :raises FileNotFoundError: If a required executable is missing.
    :raises subprocess.CalledProcessError: If a Debian packaging command fails.
    """
    build_dir = Path(args.build_dir).resolve()
    binaries = [build_dir / name for name in _REQUIRED_BINARIES]
    for binary in binaries:
        if not binary.is_file():
            message = f"Required binary not found: {binary}. Run `task core` first."
            raise FileNotFoundError(message)
    binaries.extend(build_dir / name for name in _OPTIONAL_BINARIES if (build_dir / name).is_file())

    version = f"{args.version}-{args.revision}"
    subprocess.run([_DPKG, "--validate-version", version], check=True)
    architecture = subprocess.run(
        [_DPKG, "--print-architecture"], check=True, capture_output=True, text=True
    ).stdout.strip()
    output_dir = Path(args.output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    deb_file = output_dir / f"clp-core_{version}_{architecture}.deb"

    with tempfile.TemporaryDirectory(prefix="clp-core-") as temporary_dir:
        staging_dir = Path(temporary_dir)
        package_dir = staging_dir / "package"
        bin_dir = package_dir / "usr" / "bin"
        control_dir = package_dir / "DEBIAN"
        bin_dir.mkdir(parents=True)
        control_dir.mkdir()
        for binary in binaries:
            destination = bin_dir / binary.name
            shutil.copyfile(binary, destination)
            destination.chmod(0o755)

        # dpkg-shlibdeps requires a source control file, separate from the binary package.
        source_control_dir = staging_dir / "debian"
        source_control_dir.mkdir()
        (source_control_dir / "control").write_text(
            "Source: clp-core\n\nPackage: clp-core\nArchitecture: any\n", encoding="utf-8"
        )
        dependency_output = subprocess.run(
            [_DPKG_SHLIBDEPS, "-O", *(f"-e{bin_dir / binary.name}" for binary in binaries)],
            cwd=staging_dir,
            check=True,
            capture_output=True,
            text=True,
        ).stdout
        dependencies = next(
            (
                line.removeprefix("shlibs:Depends=")
                for line in dependency_output.splitlines()
                if line.startswith("shlibs:Depends=")
            ),
            "",
        )
        control = [
            "Package: clp-core",
            "Maintainer: YScope <admin@yscope.com>",
            f"Version: {version}",
            f"Architecture: {architecture}",
            "Section: utils",
            "Priority: optional",
            "Description: CLP core compression, extraction, and search tools",
        ]
        if dependencies:
            control.append(f"Depends: {dependencies}")
        (control_dir / "control").write_text("\n".join(control) + "\n", encoding="utf-8")
        subprocess.run(
            [_DPKG_DEB, "--root-owner-group", "--build", str(package_dir), str(deb_file)],
            check=True,
        )
    return deb_file


def main() -> int:
    """Parses arguments and creates a Debian package from a completed core build."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "-b", "--build-dir", required=True, help="The CLP core CMake build directory"
    )
    parser.add_argument(
        "-v", "--version", required=True, help="The Debian package upstream version"
    )
    parser.add_argument("-r", "--revision", default="1", help="The Debian package revision")
    parser.add_argument("-o", "--output-dir", default=".", help="The package output directory")
    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")
    try:
        deb_file = create_debian_package(args)
    except (OSError, subprocess.CalledProcessError) as error:
        diagnostics = error.stderr if isinstance(error, subprocess.CalledProcessError) else ""
        logger.exception("Failed to create Debian package. %s", diagnostics or "")
        return 1
    logger.info("Package created: %s", deb_file)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
