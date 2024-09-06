# Containers

We publish (to [GitHub packages][gh-packages]) several Docker container images useful for building
and running CLP:

* An [image][core-deps-centos-stream-9] containing the dependencies necessary to build CLP core in a
  Centos Stream 9 x86 environment.

  ```text
  ghcr.io/y-scope/clp/clp-core-dependencies-x86-centos-stream-9:main
  ```

* An [image][core-deps-ubuntu-focal] containing the dependencies necessary to build CLP core in an
  Ubuntu Focal x86 environment.

  ```text
  ghcr.io/y-scope/clp/clp-core-dependencies-x86-ubuntu-focal:main
  ```

* An [image][core-deps-ubuntu-jammy] containing the dependencies necessary to build CLP core in an
  Ubuntu Jammy x86 environment.

  ```text
  ghcr.io/y-scope/clp/clp-core-dependencies-x86-ubuntu-jammy:main
  ```

* An [image][core-ubuntu-focal] containing the CLP core binaries (`clg`, `clp`, `clp-s`, `glt`,
  etc.) built in an Ubuntu Focal x86 environment.

  ```text
  ghcr.io/y-scope/clp/clp-core-x86-ubuntu-focal:main
  ```

* An [image][exe-ubuntu-focal] containing the dependencies necessary to run the CLP package in an
  Ubuntu Focal x86 environment.

  ```text
  ghcr.io/y-scope/clp/clp-execution-x86-ubuntu-focal:main
  ```

* An [image][exe-ubuntu-jammy] containing the dependencies necessary to run the CLP package in an
  Ubuntu Jammy x86 environment.

  ```text
  ghcr.io/y-scope/clp/clp-execution-x86-ubuntu-jammy:main
  ```

[core-deps-centos-stream-9]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-dependencies-x86-centos-stream-9
[core-deps-ubuntu-focal]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-dependencies-x86-ubuntu-focal
[core-deps-ubuntu-jammy]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-dependencies-x86-ubuntu-jammy
[core-ubuntu-focal]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-x86-ubuntu-focal
[exe-ubuntu-focal]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-execution-x86-ubuntu-focal
[exe-ubuntu-jammy]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-execution-x86-ubuntu-jammy
[gh-packages]: https://github.com/orgs/y-scope/packages?repo_name=clp
