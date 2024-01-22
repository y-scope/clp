<img alt="CLP" src="https://yscope.com/img/clp-logo.png" width="300"/>

[![Open bug reports](https://img.shields.io/github/issues/y-scope/clp/bug?label=bugs)](https://github.com/y-scope/clp/issues?q=is%3Aissue+is%3Aopen+label%3Abug)
[![Open feature requests](https://img.shields.io/github/issues/y-scope/clp/enhancement?label=feature-requests)](https://github.com/y-scope/clp/issues?q=is%3Aissue+is%3Aopen+label%3Aenhancement)
[![CLP on Zulip](https://img.shields.io/badge/zulip-yscope--clp%20chat-1888FA?logo=zulip)](https://yscope-clp.zulipchat.com/) 

YScope's Compressed Log Processor (CLP) is a tool capable of losslessly compressing text logs and
searching the compressed logs without decompression. To learn more about it, you can read our 
[paper](https://www.usenix.org/system/files/osdi21-rodrigues.pdf).

# Getting Started

You can download a [release package](https://github.com/y-scope/clp/releases) which includes support
for distributed compression and search. Or, to quickly try CLP's *core* compression and search, you
can use a [prebuilt container](docs/core/clp-core-container.md).

We also have guides for building the [package](docs/Building.md) and
[CLP core](components/core/README.md) from source.

For some logs you can use to test CLP, check out our open-source 
[datasets](docs/Datasets.md).

# Providing Feedback

You can use GitHub issues to [report a bug](https://github.com/y-scope/clp/issues/new?assignees=&labels=bug&template=bug-report.yml) 
or [request a feature](https://github.com/y-scope/clp/issues/new?assignees=&labels=enhancement&template=feature-request.yml).

Join us on [Zulip](https://yscope-clp.zulipchat.com/) to chat with developers 
and other community members.

# Project Structure

CLP is currently split across a few different components in the [components](components) 
directory:

* [clp-package-utils](components/clp-package-utils) contains Python utilities
  for operating the CLP package.
* [clp-py-utils](components/clp-py-utils) contains Python utilities common to several of the 
  other components.
* [core](components/core) contains code to compress uncompressed logs, decompress compressed 
  logs, and search compressed logs.
* [job-orchestration](components/job-orchestration) contains code to schedule compression jobs on
  the cluster.
* [package-template](components/package-template) contains the base directory structure and files of the 
  CLP package.

# GitHub Packages

The artifacts published to [GitHub packages][1] in this repo are a set of Docker container images
useful for building and running CLP:

| Image name                                                        | Image contents                                                                                       | Link   |
|-------------------------------------------------------------------|------------------------------------------------------------------------------------------------------|--------|
| `ghcr.io/y-scope/clp/clp-core-dependencies-x86-centos7.4:main`    | The dependencies necessary to build CLP core in a Centos 7.4 x86 environment.                        | [↗][2] |
| `ghcr.io/y-scope/clp/clp-core-dependencies-x86-ubuntu-focal:main` | The dependencies necessary to build CLP core in an Ubuntu Focal x86 environment.                     | [↗][3] |
| `ghcr.io/y-scope/clp/clp-core-dependencies-x86-ubuntu-jammy:main` | The dependencies necessary to build CLP core in an Ubuntu Jammy x86 environment.                     | [↗][4] |
| `ghcr.io/y-scope/clp/clp-core-x86-ubuntu-focal:main`              | The CLP core binaries (`clg`, `clp`, `clp-s`, `glt`, etc.) built in an Ubuntu Focal x86 environment. | [↗][5] |
| `ghcr.io/y-scope/clp/clp-execution-x86-ubuntu-focal:main`         | The dependencies necessary to run the CLP package in an x86 environment.                             | [↗][6] |

# Next Steps

This is our open-source release which we will be constantly updating with bug fixes, features, etc.
If you would like a feature or want to report a bug, please file an issue and we'll be happy to engage.

# Contributing

Have an issue you want to fix or a feature you'd like to implement? We'd love to see it!

## Linting

Before submitting a PR, ensure you've run our linting tools and either fixed any violations or
suppressed the warning. To run our linting workflows locally, you'll need [Task][7]. Alternatively,
you can run the [clp-lint](.github/workflows/clp-lint.yaml) workflow in your fork.

To perform the linting checks:

```shell
task lint-check
```

To also apply any automatic fixes:

```shell
task lint-fix
```

[1]: https://github.com/orgs/y-scope/packages?repo_name=clp
[2]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-dependencies-x86-centos7.4
[3]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-dependencies-x86-ubuntu-focal
[4]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-dependencies-x86-ubuntu-jammy
[5]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-x86-ubuntu-focal
[6]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-execution-x86-ubuntu-focal
[7]: https://taskfile.dev/
