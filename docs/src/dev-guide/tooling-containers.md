# Containers

We publish (to [GitHub packages][gh-packages]) several Docker container images useful for building
and running CLP:

* An [image][1] containing the dependencies necessary to build CLP core in a Centos 7.4 x86
  environment.

  ```text
  ghcr.io/y-scope/clp/clp-core-dependencies-x86-centos7.4:main
  ```

* An [image][2] containing the dependencies necessary to build CLP core in an Ubuntu Focal x86
  environment.

  ```text
  ghcr.io/y-scope/clp/clp-core-dependencies-x86-ubuntu-focal:main
  ```

* An [image][3] containing the dependencies necessary to build CLP core in an Ubuntu Jammy x86
  environment.

  ```text
  ghcr.io/y-scope/clp/clp-core-dependencies-x86-ubuntu-jammy:main
  ```

* An [image][4] containing the CLP core binaries (`clg`, `clp`, `clp-s`, `glt`, etc.) built in an
  Ubuntu Focal x86 environment.

  ```text
  ghcr.io/y-scope/clp/clp-core-x86-ubuntu-focal:main
  ```

* An [image][5] containing the dependencies necessary to run the CLP package in an Ubuntu Focal x86
  environment.

  ```text
  ghcr.io/y-scope/clp/clp-execution-x86-ubuntu-focal:main
  ```

* An [image][6] containing the dependencies necessary to run the CLP package in an Ubuntu Jammy x86
  environment.

  ```text
  ghcr.io/y-scope/clp/clp-execution-x86-ubuntu-jammy:main
  ```

[gh-packages]: https://github.com/orgs/y-scope/packages?repo_name=clp
[1]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-dependencies-x86-centos7.4
[2]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-dependencies-x86-ubuntu-focal
[3]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-dependencies-x86-ubuntu-jammy
[4]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-x86-ubuntu-focal
[5]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-execution-x86-ubuntu-focal
[6]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-execution-x86-ubuntu-jammy

