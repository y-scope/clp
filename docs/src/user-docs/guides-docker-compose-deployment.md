# Docker Compose deployment

This guide explains how to deploy CLP using Docker Compose. Docker Compose provides a
straightforward way to orchestrate CLP's services, suitable for both development and production
environments.

A Docker Compose deployment runs all CLP services on a single host. The
[quick-start guides](quick-start/index.md) explain how to set up and start this deployment.

:::{note}
Docker Compose deployments across multiple hosts are not supported in the current release. To run
CLP across multiple hosts, use [Kubernetes][kubernetes-guide] (via Helm).
:::

For a detailed overview of all services and their dependencies, see the
[deployment orchestration design doc][design-orchestration].

## Using CLP

To learn how to compress and search your logs, check out the quick-start guide that corresponds to
the flavor of CLP you're running:

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: quick-start/clp-json
Using clp-json
^^^
How to compress and search JSON logs.
:::

:::{grid-item-card}
:link: quick-start/clp-text
Using clp-text
^^^
How to compress and search unstructured text logs.
:::
::::

## Stopping CLP

To stop CLP, run:

```bash
sbin/stop-clp.sh
```

This will stop all CLP services managed by Docker Compose on the current host.

## Monitoring and debugging

First, determine your instance ID from `<clp-package>/var/log/instance-id`.

To check the status of services:

```bash
docker compose --project-name clp-package-<instance-id> ps
```

To view logs for a specific service:

```bash
docker compose --project-name clp-package-<instance-id> logs -f <service-name>
```

To execute commands in a running container:

```bash
docker compose --project-name clp-package-<instance-id> exec <service-name> /bin/bash
```

To validate your Docker Compose configuration:

```bash
docker compose config
```

[design-orchestration]: ../dev-docs/design-deployment-orchestration.md
[kubernetes-guide]: guides-k8s-deployment.md
