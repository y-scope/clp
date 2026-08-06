# Containers

We maintain several Docker container images that are useful for building and running CLP. All images
can be built and used locally, but some are available to download from
[GitHub Packages][gh-packages].

To build an image locally, run the `build.sh` script in the image's directory.

## clp-core-dependencies-manylinux_2_28

Image containing the dependencies necessary to build CLP core in a [manylinux_2_28][manylinux_2_28]
environment. It is published as a multi-arch image (amd64 and arm64); Docker auto-selects the
architecture matching the host on pull.

Binaries built on manylinux_2_28 (based on AlmaLinux 8) are expected to be compatible with other
distros using glibc 2.28+, including:

* CentOS/RHEL 8+
* Debian 10+
* Fedora 29+
* Ubuntu 18.10+

* [GitHub Packages page][core-deps-manylinux_2_28]
* Pull command:

  ```bash
  docker pull ghcr.io/y-scope/clp/clp-core-dependencies-manylinux_2_28:main
  ```

* Path:

  ```text
  components/core/tools/docker-images/clp-env-base-manylinux_2_28
  ```

  To cross-build for a non-native architecture locally (using QEMU emulation), set the `PLATFORM`
  env var when running `build.sh`, e.g. `PLATFORM=linux/arm64 ./build.sh`
  (requires binfmt/QEMU support on the host).

  Local builds keep the architecture-specific `:dev` tags used by the packaging scripts:
  `clp-core-dependencies-x86-manylinux_2_28:dev` for amd64 and
  `clp-core-dependencies-aarch64-manylinux_2_28:dev` for arm64.

## clp-core-dependencies-musllinux_1_2

Image containing the dependencies necessary to build CLP core in a [musllinux_1_2][musllinux_1_2]
environment. It is published as a multi-arch image (amd64 and arm64); Docker auto-selects the
architecture matching the host on pull.

Binaries built on musllinux_1_2 (based on Alpine Linux 3.22) are expected to be compatible with
other distros using musl 1.2, including:

* Alpine Linux 3.13+

* [GitHub Packages page][core-deps-musllinux_1_2]
* Pull command:

  ```bash
  docker pull ghcr.io/y-scope/clp/clp-core-dependencies-musllinux_1_2:main
  ```

* Path:

  ```text
  components/core/tools/docker-images/clp-env-base-musllinux_1_2
  ```

  To cross-build for a non-native architecture locally (using QEMU emulation), set the `PLATFORM`
  env var when running `build.sh`, e.g. `PLATFORM=linux/arm64 ./build.sh`
  (requires binfmt/QEMU support on the host).

  Local builds keep the architecture-specific `:dev` tags used by the packaging scripts:
  `clp-core-dependencies-x86-musllinux_1_2:dev` for amd64 and
  `clp-core-dependencies-aarch64-musllinux_1_2:dev` for arm64.

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

## clp-package

An image containing the CLP package.

* Path:

  ```text
  tools/docker-images/clp-package
  ```

### Notable configurations

The container image sets the following environment variables and configurations, some of which
(e.g., the user's home directory) may be unexpected.

<table class="table">
  <thead>
    <tr>
      <th>Configuration</th>
      <th>Value</th>
      <th>Description</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code class="literal">$USER</code></td>
      <td><code class="literal">clp-user</code></td>
      <td>Default user in the container.</td>
    </tr>
    <tr>
      <td><code class="literal">$CLP_HOME</code> and <code class="literal">$HOME</code>
        (<code class="literal">~</code>)</td>
      <td><code class="literal">/opt/clp</code></td>
      <td>Home directory for the default user.</td>
    </tr>
    <tr>
      <td><code class="literal">$SHELL</code></td>
      <td><code class="literal">/bin/bash</code></td>
      <td>Default shell for the default user.</td>
    </tr>
    <tr>
      <td><code class="literal">$LD_LIBRARY_PATH</code></td>
      <td><code class="literal">${CLP_HOME}/lib</code></td>
      <td>Library path to include any shared libraries in the CLP package.</td>
    </tr>
    <tr>
      <td><code class="literal">$PATH</code></td>
      <td><code class="literal">${CLP_HOME}/sbin</code><br>
        <code class="literal">:${CLP_HOME}/bin</code><br>
        <code class="literal">:${PATH}</code></td>
      <td>System path including the binaries and scripts in the CLP package.</td>
    </tr>
    <tr>
      <td><code class="literal">$PYTHONPATH</code></td>
      <td><code class="literal">${CLP_HOME}/lib/python3/site-packages</code></td>
      <td>Python path to include any Python packages in the CLP package.</td>
    </tr>
  </tbody>
</table>

## Corporate proxy and mirror support

All base image `build.sh` scripts source `docker-image-build.sh`, which wraps [yscope-dev-utils'
`docker/build` and `docker/ca-trust` libraries][ca-trust] to support building behind corporate
proxies and with custom package mirrors. With no flags or environment variables set, builds behave
as they do without this support.

This adds no Docker version requirement. Named build contexts are a Buildx feature, available since
Buildx 0.8 — well below the `docker-buildx-plugin` >= 0.15.1 that CLP already requires. See
[Requirements](building-package.md#requirements).

### Corporate proxy (TLS-intercepting)

Where a TLS-intercepting proxy (e.g., Zscaler, Fortinet, Palo Alto) replaces upstream SSL
certificates, pass `--with-ca-certs` to `build.sh`. The host's CA bundle is then mounted into the
build steps that reach the network, so downloads can verify the proxy's certificates without
disabling SSL. The bundle is taken from `SSL_CERT_FILE` when set, else from common CA-bundle
locations; expired certificates are dropped.

`curl`, `pip`, and `apk` pick the bundle up from the environment variables the library exports.
`apt` and `dnf` read neither `SSL_CERT_FILE` nor `CURL_CA_BUNDLE`, so they are pointed at it
explicitly (`Acquire::https::CaInfo` and `--setopt=sslcacert`) by
`lib_install/ca-trust-pkg-opts.sh`. A tool that reads none of these — `wget`, Node
(`NODE_EXTRA_CA_CERTS`), or a rustls-based downloader — would not be covered; none is used by these
images today.

Host CA trust is **opt-in and never baked into an image**. The bundle is mounted only for the
duration of each build step, so it appears in no image layer and in no `docker history` entry, and
published images keep their own distro trust store. Builds without the flag — including every CI
build — do no CA handling at all.

With a local builder, the bundle persists in the BuildKit cache as a `source.local` entry
(visible via `docker buildx du`) until `docker builder prune`.

That locality isn't guaranteed in general. A remote or containerized builder receives the build
context over the wire, and cache export (`--cache-to`) can copy build data to a registry or another
host. Don't pass `--with-ca-certs` to a builder you don't control, or with cache export enabled,
unless you're willing for the bundle to reach it. `docker buildx ls` shows which builder is active.

Because nothing is baked in, a corporate gateway also needs the bundle at *run* time for commands
that reach the network from inside a container. `run-in-container.sh` takes the same
`--with-ca-certs` flag for that.

The following proxy environment variables are forwarded as Docker build args when set:

| Variable                      | Description                             |
|-------------------------------|-----------------------------------------|
| `HTTP_PROXY` / `http_proxy`   | HTTP proxy URL                          |
| `HTTPS_PROXY` / `https_proxy` | HTTPS proxy URL                         |
| `ALL_PROXY` / `all_proxy`     | SOCKS/catch-all proxy URL               |
| `NO_PROXY` / `no_proxy`       | Comma-separated list of hosts to bypass |

When a proxy URL points to localhost (`127.0.0.1`, `localhost`, or `[::1]`), `--network host` is
automatically added to the Docker build so the container can reach the host's proxy. This can be
overridden with `DOCKER_NETWORK`.

### Package mirror overrides

Each distro supports an environment variable to override the default package mirror:

| Variable              | Distro                                      | Example                                  |
|-----------------------|---------------------------------------------|------------------------------------------|
| `DNF_MIRROR_BASE_URL` | manylinux_2_28 (AlmaLinux), centos-stream-9 | `https://internal.example.com/almalinux` |
| `APK_MIRROR_URL`      | musllinux_1_2 (Alpine)                      | `https://internal.example.com/alpine`    |
| `APT_MIRROR_URL`      | ubuntu-jammy                                | `https://internal.example.com/ubuntu`    |

### Other build flags

| Variable         | Default | Description                                                                    |
|------------------|---------|--------------------------------------------------------------------------------|
| `DOCKER_PULL`    | `true`  | Pull the latest base image before building. Set to `false` for offline builds. |
| `DOCKER_NETWORK` | (auto)  | Override Docker's network mode (e.g., `host`, `bridge`).                       |

[ca-trust]: https://github.com/y-scope/yscope-dev-utils/blob/main/exports/docker/ca-trust/README.md
[core-deps-centos-stream-9]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-dependencies-x86-centos-stream-9
[core-deps-manylinux_2_28]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-dependencies-manylinux_2_28
[core-deps-musllinux_1_2]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-dependencies-musllinux_1_2
[core-deps-ubuntu-jammy]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-dependencies-x86-ubuntu-jammy
[core-ubuntu-jammy]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-x86-ubuntu-jammy
[gh-packages]: https://github.com/orgs/y-scope/packages?repo_name=clp
[manylinux_2_28]: https://github.com/pypa/manylinux?tab=readme-ov-file#manylinux_2_28-almalinux-8-based
[musllinux_1_2]: https://github.com/pypa/manylinux?tab=readme-ov-file#musllinux_1_2-alpine-linux-322-based-313-compatible
