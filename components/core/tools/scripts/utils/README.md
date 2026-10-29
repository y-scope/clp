This directory contains uncategorized utility scripts.


* `build-and-run-unit-tests.sh` can be used to build all executables and run
  the unit tests.
* `run-in-container.sh` can be used to run a command (e.g., `make`), in a 
  container containing the core component's dependencies.
  * All commands are run from the root of the core component.
  * This is useful if, for instance, you're developing on Ubuntu X, but the
    package uses Ubuntu Y; you can build in the Ubuntu Y container to avoid
    compatibility issues.
* `create-debian-package.py` generates a minimal Debian package containing the CLP core binaries.
  Build with `task core` on the target Debian/Ubuntu release and install `dpkg-dev` before packaging.
  Runtime dependencies are detected from the built executables. The optional `glt` and
  `make-dictionaries-readable` executables are included if present in the build directory.
  Run from the repository root, for example:
  `python3 components/core/tools/scripts/utils/create-debian-package.py --version 0.14.0 --revision 1 --build-dir build/core`
