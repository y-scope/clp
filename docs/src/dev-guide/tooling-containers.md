# Containers

We maintain several Docker container images that are useful for building and running CLP. All images
can be built and used locally, but some are available to download from
[GitHub Packages][gh-packages].

To build an image locally, run the `build.sh` script in the image's directory.

## clp-core-dependencies-&lt;arch&gt;-manylinux_2_28

Images containing the dependencies necessary to build CLP core in a [manylinux_2_28][manylinux_2_28]
environment (aarch64 or x86).

Binaries built on manylinux_2_28 (based on AlmaLinux 8) are expected to be compatible with other
distros using glibc 2.28+, including:

* CentOS/RHEL 8+
* Debian 10+
* Fedora 29+
* Ubuntu 18.10+

### clp-core-dependencies-aarch64-manylinux_2_28

* Path:

  ```text
  components/core/tools/docker-images/clp-env-base-manylinux_2_28-aarch64
  ```

### clp-core-dependencies-x86-manylinux_2_28

* Path:

  ```text
  components/core/tools/docker-images/clp-env-base-manylinux_2_28-x86_64
  ```

## clp-core-dependencies-x86-centos-stream-9

An image containing the dependencies necessary to build CLP core in a CentOS Stream 9 x86
environment.

* [GitHub Packages page][core-deps-centos-stream-9]
* Pull command:

  ```bash
  docker pull ghcr.io/y-scope/clp/clp-core-dependencies-x86-centos-stream-9:main
  ```

* Path:

  ```text
  components/core/tools/docker-images/clp-env-base-centos-stream-9
  ```

## clp-core-dependencies-x86-ubuntu-jammy

An image containing the dependencies necessary to build CLP core in an Ubuntu Jammy x86
environment.

* [GitHub Packages page][core-deps-ubuntu-jammy]
* Pull command:

  ```bash
  docker pull ghcr.io/y-scope/clp/clp-core-dependencies-x86-ubuntu-jammy:main
  ```

* Path:

  ```text
  components/core/tools/docker-images/clp-env-base-ubuntu-jammy
  ```

## clp-core-x86-ubuntu-jammy

An image containing the CLP core binaries (`clg`, `clp`, `clp-s`, `glt`, etc.) built in an Ubuntu
Jammy x86 environment.

* [GitHub Packages page][core-ubuntu-jammy]
* Pull command:

  ```bash
  docker pull ghcr.io/y-scope/clp/clp-core-x86-ubuntu-jammy:main
  ```

* Path:

  ```text
  components/core/tools/docker-images/clp-core-ubuntu-jammy
  ```

## clp-execution-x86-ubuntu-jammy

An image containing the dependencies necessary to run the CLP package in an Ubuntu Jammy x86
environment.

* [GitHub Packages page][exe-ubuntu-jammy]
* Pull command:

  ```bash
  docker pull ghcr.io/y-scope/clp/clp-execution-x86-ubuntu-jammy:main
  ```

* Path:

  ```text
  tools/docker-images/clp-execution-base-ubuntu-jammy
  ```

[core-deps-centos-stream-9]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-dependencies-x86-centos-stream-9
[core-deps-ubuntu-jammy]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-dependencies-x86-ubuntu-jammy
[core-ubuntu-jammy]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-x86-ubuntu-jammy
[exe-ubuntu-jammy]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-execution-x86-ubuntu-jammy
[gh-packages]: https://github.com/orgs/y-scope/packages?repo_name=clp
[manylinux_2_28]: https://github.com/pypa/manylinux?tab=readme-ov-file#manylinux_2_28-almalinux-8-based
