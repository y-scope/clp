# Building and testing the Presto connector

The Presto integration installs the [CLP connector][clp-connector] at startup from the
`ghcr.io/y-scope/clp-plugin-presto-connector` image, so that the stock Presto images can be used
unmodified. This page covers building that connector image locally and pointing the
[Docker Compose](#docker-compose) and [Helm](#helm-kind) stacks at it for testing.

The connector image is published as a multi-architecture manifest, but a multi-architecture manifest
can't be loaded into a local Docker daemon. Local builds therefore produce, and the stacks consume,
a per-architecture tag of the form `<version>-<arch>` (e.g. `0.1.0-SNAPSHOT-arm64`).

## Building the connector image

In the [`clp-plugin-presto-connector`][clp-connector] repository, run:

```shell
task package
```

This builds the connector image and loads it into the local Docker daemon under the
per-architecture tag, e.g. `ghcr.io/y-scope/clp-plugin-presto-connector:0.1.0-SNAPSHOT-arm64`.

## Docker Compose

The `presto-clp` Compose stack is in `tools/deployment/presto-clp`. `scripts/set-up-config.sh`
resolves the connector tag into `.env`, trying in order:

1. A locally-built per-architecture tag (`<version>-<arch>`, via `docker image inspect`).
2. A published multi-architecture tag (`<version>`, via `docker manifest inspect`).
3. A published per-architecture tag (`<version>-<arch>`).

If none is found, it errors with the refs it tried. Set `CLP_CONNECTOR_PREFER_LOCAL=false` to flip
the order and test the published multi-architecture image even when a local build of the same
version is loaded. Override the resolution by exporting environment variables before running
`set-up-config.sh`:

* `CLP_CONNECTOR_IMAGE` (default `ghcr.io/y-scope/clp-plugin-presto-connector`): connector image
  repository.
* `CLP_CONNECTOR_TAG`: exact tag; skips resolution when set.
* `CLP_CONNECTOR_VERSION` (default `0.1.0-SNAPSHOT`): version to resolve (multi-arch tag / per-arch
  prefix).
* `CLP_CONNECTOR_PREFER_LOCAL` (default `true`): set to `false` to prefer the published multi-arch
  image over a local build.

Then start the stack:

```shell
./scripts/set-up-config.sh <clp-package-dir>
docker compose up -d
```

See the [Using Presto with CLP][using-presto] user guide for the full setup.

## Helm (kind)

The Helm chart's local-image path is wired into the set-up scripts in
`tools/deployment/package-helm`, which load a local image into the `kind` cluster and set
`image.clpConnector.{repository,tag,pullPolicy=Never}` for you. Pass the connector image via
`--clp-connector-image`, using the per-architecture tag:

```shell
./package-helm/set-up-test.sh --presto --clp-connector-image=<repo>:<version>-<arch>
```

For example:

```shell
./package-helm/set-up-test.sh --presto \
    --clp-connector-image=ghcr.io/y-scope/clp-plugin-presto-connector:0.1.0-SNAPSHOT-arm64
```

[clp-connector]: https://github.com/y-scope/clp-plugin-presto-connector
[using-presto]: ../user-docs/guides-using-presto.md
