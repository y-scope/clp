This directory contains uncategorized utility scripts.

* `build.py` can be used to trigger a local platform build
* `build-with-docker.py` can be used to build all targets within a docker 
  image for x86_64 and arm64
    * manylinux_2_28 based images are used and the output binaries are 
      expected to be compatible with Debian 10+, Ubuntu 18.10+, Fedora 29+,
      CentOS/RHEL 8+ and other platforms.
    * Command to enable docker emulation: 
      `docker run --rm --privileged multiarch/qemu-user-static --reset -p yes`
* `build-and-run-unit-tests.py` can be used to perform a local platform 
  build of all executables and run the unit tests.
* `run-in-container.sh` can be used to run a command (e.g., `make`), in a 
  container containing the core component's dependencies.
  * All commands are run from the root of the core component.
  * This is useful if, for instance, you're developing on Ubuntu X, but the
    package uses Ubuntu Y; you can build in the Ubuntu Y container to avoid
    compatibility issues.
