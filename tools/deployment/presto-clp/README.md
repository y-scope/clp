# presto-clp

Docker Compose deployment of Presto with the [CLP Presto connector][clp-connector] installed at
startup, for querying a running [CLP package][clp-package]'s archives from Presto.

The coordinator and worker run the stock `ghcr.io/y-scope/presto` and
`ghcr.io/y-scope/presto-native` images (unmodified); the CLP connector plugin is installed into
shared volumes by a one-shot `clp-plugin-presto-connector-init` service before Presto starts.

## Setup

This stack runs alongside a CLP package and reads its config, so start the CLP package first, then
generate `.env` from the package's config:

```sh
./scripts/set-up-config.sh <clp-package-dir>
```

This creates a Python venv, renders the Presto config from `clp-config.yaml` / `credentials.yaml`,
and resolves the connector image tag (see below) into `.env`. Then start the stack:

```sh
docker compose up -d
```

The coordinator UI is exposed on the host at `http://localhost:8889`.

## Connector image resolution

`init.py` writes `CLP_CONNECTOR_IMAGE` and `CLP_CONNECTOR_TAG` into `.env`, which
`docker-compose.yaml` consumes. By default the tag is resolved by trying, in order:

1. A **locally-built per-architecture** tag `<version>-<arch>` (via `docker image inspect`) — so a
   connector built locally via `task package` is picked up with no network lookup.
2. A **published multi-architecture** tag `<version>` (via `docker manifest inspect`); each node
   pulls its own architecture from the manifest.
3. A **published per-architecture** tag `<version>-<arch>`.

If none is found, the setup errors out with the refs it tried and instructions to build the
connector or set `CLP_CONNECTOR_TAG` explicitly.

To test the **published/upstream** image even when a local build of the same version is loaded, set
`CLP_CONNECTOR_PREFER_LOCAL=false` — this tries the published multi-architecture tag first.

Override the resolution by exporting environment variables before running `set-up-config.sh`:

| Variable                    | Default                                           | Purpose                                            |
| --------------------------- | ------------------------------------------------- | ------------------------------------------------- |
| `CLP_CONNECTOR_IMAGE`       | `ghcr.io/y-scope/clp-plugin-presto-connector`     | Connector image repository.                       |
| `CLP_CONNECTOR_TAG`        | (resolved)                                        | Exact tag; skips resolution when set.             |
| `CLP_CONNECTOR_VERSION`    | `0.1.0-SNAPSHOT`                                  | Version to resolve (multi-arch tag / per-arch prefix). |
| `CLP_CONNECTOR_PREFER_LOCAL`| `true`                                            | `false` prefers the published multi-arch image over a local build. |

## Building the connector locally

In the [`clp-plugin-presto-connector`][clp-connector] repo, `task package` builds and loads the
connector image into the local Docker daemon under the per-architecture tag, e.g.

`ghcr.io/y-scope/clp-plugin-presto-connector:0.1.0-SNAPSHOT-arm64`

A multi-architecture manifest can't be loaded into a local daemon, so the per-architecture tag is
what local builds produce and what Compose picks up in step 1 above — no manual retag needed.

## Helm (kind) local testing

The Helm chart's local-image path is wired into the set-up scripts in
[`tools/deployment/package-helm`](../package-helm), which load a local image into the kind cluster
and set `image.clpConnector.{repository,tag,pullPolicy=Never}` for you. Pass the connector image via
`--clp-connector-image`, using the per-architecture tag (for the same local-daemon reason):

```sh
./package-helm/set-up-test.sh --presto --clp-connector-image=<repo>:<version>-<arch>
```

For example, `ghcr.io/y-scope/clp-plugin-presto-connector:0.1.0-SNAPSHOT-arm64`.

[clp-connector]: https://github.com/y-scope/clp-plugin-presto-connector
[clp-package]: https://github.com/y-scope/clp/tree/main/tools/deployment/package