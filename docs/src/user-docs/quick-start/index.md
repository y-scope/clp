# Overview

This guide describes the following:

* [CLP's system requirements](#system-requirements)
* [How to choose a CLP flavor](#choosing-a-flavor)
* [How to use CLP](#using-clp)

---

## System Requirements

This quick start guide covers **single-host** deployment using Docker Compose or Kubernetes with 
Helm. For deployments that scale across multiple machines for higher throughput, see:

* [Docker Compose deployment][docker-compose-deployment] for advanced Docker Compose configurations
* [Kubernetes deployment][k8s-deployment] for production Kubernetes clusters

Choose the requirements below based on your preferred orchestration method.

::::{tab-set}
:::{tab-item} Docker Compose
:sync: docker

* [Docker][Docker]
  * `containerd.io` >= 1.7.18
  * `docker-ce` >= 27.0.3
  * `docker-ce-cli` >= 27.0.3
  * `docker-compose-plugin` >= 2.28.1

To check whether the required tools are installed on your system, run:

```bash
containerd --version
docker version --format '{{.Server.Version}}'
docker compose version --short
```

```{note}
* If you're not running as root, ensure Docker can be run
  [without superuser privileges][docker-non-root].
* If you're using Docker Desktop, ensure version 4.34 or higher is installed.
```

:::

:::{tab-item} Kubernetes (`kind`)
:sync: kind

[`kind`][kind] (Kubernetes in Docker) runs a Kubernetes cluster inside Docker containers, making it
ideal for local Kubernetes testing and development.

* [Docker][Docker] (required for `kind`)
  * `containerd.io` >= 1.7.18
  * `docker-ce` >= 27.0.3
  * `docker-ce-cli` >= 27.0.3
* [`kubectl`][kubectl] >= 1.30
* [Helm][Helm] >= 4.0
* [`kind`][kind] >= 0.23

To check whether the tools are installed on your system, run:

```bash
containerd --version
docker version --format '{{.Server.Version}}'
kubectl version --client --output=yaml | grep gitVersion
helm version --short
kind version
```

:::
::::

---

## Choosing a flavor

There are two flavors of CLP:

* **[clp-json](#clp-json)** for compressing and searching **JSON** logs.
* **[clp-text](#clp-text)** for compressing and searching **unstructured text** logs.

:::{note}
Both flavors contain the same binaries but are configured with different values for the
`package.storage_engine` key in the package's config file (`etc/clp-config.yaml`).
:::

### clp-json

The JSON flavor of CLP is appropriate for JSON logs, where each log event is an independent JSON
object. For example:

```json lines
{
  "t": {
    "$date": "2023-03-21T23:46:37.392"
  },
  "ctx": "conn11",
  "msg": "Waiting for write concern."
}
{
  "t": {
    "$date": "2023-03-21T23:46:37.392"
  },
  "msg": "Set last op to system time"
}
```

The log file above contains two log events represented by two JSON objects printed one after the
other. Whitespace is ignored, so the log events could also appear with no newlines and indentation.

If you're using JSON logs, download and extract the `clp-json` release from the
[Releases][clp-releases] page, then proceed to the [`clp-json` quick-start][clp-json] guide.

### clp-text

The text flavor of CLP is appropriate for unstructured text logs, where each log event contains a
timestamp and may span one or more lines.

:::{note}
If your logs don't contain timestamps or CLP can't automatically parse the timestamps in your logs,
it will treat each line as an independent log event.
:::

For example:

```text
2015-03-23T15:50:17.926Z INFO container_1 Transitioned from ALLOCATED to ACQUIRED
2015-03-23T15:50:17.927Z ERROR Scheduler: Error trying to assign container token
java.lang.IllegalArgumentException: java.net.UnknownHostException: i-e5d112ea
    at org.apache.hadoop.security.buildTokenService(SecurityUtil.java:374)
    at org.apache.hadoop.ipc.Server$Handler.run(Server.java:2033)
Caused by: java.net.UnknownHostException: i-e5d112ea
    ... 17 more
```

The log file above contains two log events, both beginning with a timestamp. The first is a single
line, while the second contains multiple lines.

If you're using unstructured text logs, download and extract the `clp-text` release from the
[Releases][clp-releases] page, then proceed to the [`clp-text` quick-start][clp-text] guide.

---

## Using CLP

Once you've installed CLP's requirements and downloaded a CLP release, proceed to the quick-start
guide for your chosen flavor by clicking the corresponding link below.

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: clp-json
CLP for JSON logs
^^^
How to compress and search JSON logs.
:::

:::{grid-item-card}
:link: clp-text
CLP for unstructured text logs
^^^
How to compress and search unstructured text logs.
:::
::::

[clp-json]: ./clp-json.md
[clp-releases]: https://github.com/y-scope/clp/releases
[clp-text]: ./clp-text.md
[Docker]: https://docs.docker.com/engine/install/
[docker-compose-deployment]: ../guides-docker-compose-deployment.md
[docker-non-root]: https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user
[Helm]: https://helm.sh/docs/intro/install/
[k8s-deployment]: ../guides-k8s-deployment.md
[kind]: https://kind.sigs.k8s.io/docs/user/quick-start/#installation
[kubectl]: https://kubernetes.io/docs/tasks/tools/
