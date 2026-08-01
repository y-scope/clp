# Building and testing the Presto connector

The Presto integration installs the [CLP connector][clp-connector] at startup from the
`ghcr.io/y-scope/clp-plugin-presto-connector` image, so that the stock Presto images can be used
unmodified. This page covers building that connector image locally and pointing the
[Docker Compose](#docker-compose) and [Helm](#helm-kind) stacks at it for testing.

Locally-built and published connector images share the same `:<version>` tag (e.g.
`0.1.0-SNAPSHOT`), following the conventional Docker pattern: whatever is in your local Docker
daemon wins, and Docker pulls the published image when nothing is loaded locally. To go back to
the published image after building locally, `docker pull` the tag (or `docker rmi` your local
build).

## Building the connector image

In the [`clp-plugin-presto-connector`][clp-connector] repository, run:

```shell
task package
```

This builds the connector image and loads it into the local Docker daemon, e.g.
`ghcr.io/y-scope/clp-plugin-presto-connector:0.1.0-SNAPSHOT`.

## Docker Compose

The `presto-clp` Compose stack is in `tools/deployment/presto-clp`. `scripts/set-up-config.sh`
verifies the connector image exists (locally or on the registry) and writes it into `.env`,
erroring with the ref it tried if neither exists. Override the image by exporting environment
variables before running `set-up-config.sh`:

* `CLP_PRESTO_CONNECTOR_IMAGE` (default `ghcr.io/y-scope/clp-plugin-presto-connector`): connector image
  repository.
* `CLP_PRESTO_CONNECTOR_TAG`: exact tag; skips the existence check when set.
* `CLP_PRESTO_CONNECTOR_VERSION` (default `0.1.0-SNAPSHOT`): tag to verify and use.

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
`--clp-connector-image`:

```shell
tools/deployment/package-helm/set-up-test.sh --presto --clp-connector-image <repo>:<version>
```

For example:

```shell
tools/deployment/package-helm/set-up-test.sh --presto \
    --clp-connector-image ghcr.io/y-scope/clp-plugin-presto-connector:0.1.0-SNAPSHOT
```

[clp-connector]: https://github.com/y-scope/clp-plugin-presto-connector
[using-presto]: ../user-docs/guides-using-presto.md
