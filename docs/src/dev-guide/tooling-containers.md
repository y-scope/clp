# Containers

We maintain several Docker container images that are useful for building and running CLP. All images
can be built and used locally, but some are available to download from
[GitHub Packages][gh-packages].

To build an image locally, run the `build.sh` script in the image's directory.

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
