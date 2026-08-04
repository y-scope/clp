# Building and testing the Presto connector

The Presto integration installs the [CLP connector][clp-connector] at startup from the
`ghcr.io/y-scope/clp-plugin-presto-connector` image, so the stock Presto images stay unmodified.
This page covers building that image locally and pointing the [Docker Compose](#docker-compose) and
[Helm](#helm-kind) stacks at it.

The default connector image is pinned by digest, so a local build of the same tag won't be picked
up on its own — point `CLP_PRESTO_CONNECTOR_REF` at your image to use it.

## Building the connector image

In the [`clp-plugin-presto-connector`][clp-connector] repository, run:

```shell
task package
```

This builds `ghcr.io/y-scope/clp-plugin-presto-connector:0.1.0-SNAPSHOT` and loads it into your
local Docker daemon.

## Docker Compose

The `presto-clp` stack is in `tools/deployment/presto-clp`. `scripts/set-up-config.sh` verifies the
connector image exists (locally or on the registry) and writes it into `.env`, erroring with the
ref it tried if neither exists.

Each image is selected by a single environment variable holding a complete reference, which is used
as-is. Any of `repository:tag`, `repository@digest`, or `repository:tag@digest` works; unset
variables fall back to the pinned defaults.

* `CLP_PRESTO_CONNECTOR_REF`: the CLP connector.
* `CLP_PRESTO_COORDINATOR_REF`: the Presto coordinator.
* `CLP_PRESTO_WORKER_REF`: the Presto worker.

To run against the image you just built:

```shell
CLP_PRESTO_CONNECTOR_REF=ghcr.io/y-scope/clp-plugin-presto-connector:0.1.0-SNAPSHOT \
    ./scripts/set-up-config.sh <clp-package-dir>
docker compose up -d
```

Omitting the digest is what makes the local image win: Docker resolves a tag against the local
daemon first, but a digest only ever matches the exact published image.

See the [Using Presto with CLP][using-presto] user guide for the full setup.

## Helm (kind)

`--clp-connector-image` loads a local image into the `kind` cluster and sets
`image.clpConnector.{repository,tag,pullPolicy=Never}` for you:

```shell
tools/deployment/package-helm/set-up-test.sh --presto \
    --clp-connector-image ghcr.io/y-scope/clp-plugin-presto-connector:0.1.0-SNAPSHOT
```

## Pinning a new default

When bumping a pinned default, take the digest of the *manifest list*, not of a per-architecture
manifest — the connector is published for both `linux/amd64` and `linux/arm64`, and a per-arch
digest would break the other architecture:

```shell
docker buildx imagetools inspect <ref> --format '{{.Manifest.Digest}}'
```

`docker inspect` reports the digest of the single-platform image you happen to have pulled, so it's
the wrong source here.

[clp-connector]: https://github.com/y-scope/clp-plugin-presto-connector
[using-presto]: ../user-docs/guides-using-presto.md
