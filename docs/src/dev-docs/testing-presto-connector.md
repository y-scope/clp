# Building and testing the Presto connector

The Presto integration installs the [CLP connector][clp-connector] at startup from the
`ghcr.io/y-scope/clp-plugin-presto-connector` image, so the stock Presto images stay unmodified.
This page covers building that image locally and pointing the [Docker Compose](#docker-compose) and
[Helm](#helm-kind) stacks at it.

Local and published images share the same `:<version>` tag (e.g. `0.1.0-SNAPSHOT`), so whatever is
in your Docker daemon wins. `docker rmi` your local build to go back to the published image.

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
ref it tried if neither exists. Export any of these before running it:

* `CLP_PRESTO_CONNECTOR_IMAGE`: repository (default `ghcr.io/y-scope/clp-plugin-presto-connector`).
* `CLP_PRESTO_CONNECTOR_VERSION`: tag to verify and use (default `0.1.0-SNAPSHOT`).
* `CLP_PRESTO_CONNECTOR_TAG`: exact tag, skipping the existence check.

Then start the stack:

```shell
./scripts/set-up-config.sh <clp-package-dir>
docker compose up -d
```

See the [Using Presto with CLP][using-presto] user guide for the full setup.

## Helm (kind)

`--clp-connector-image` loads a local image into the `kind` cluster and sets
`image.clpConnector.{repository,tag,pullPolicy=Never}` for you:

```shell
tools/deployment/package-helm/set-up-test.sh --presto \
    --clp-connector-image ghcr.io/y-scope/clp-plugin-presto-connector:0.1.0-SNAPSHOT
```

[clp-connector]: https://github.com/y-scope/clp-plugin-presto-connector
[using-presto]: ../user-docs/guides-using-presto.md
