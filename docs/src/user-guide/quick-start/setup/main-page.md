# Setup

This page will guide you through CLP's system requirements, and help you choose which flavour of CLP will be best for you.

---

## System Requirements

To run properly on your system, CLP requires a few other programs. Before you set up CLP, ensure that you have the following installed.

### Docker

CLP uses Docker to deploy its different components. You can check whether Docker is already installed on your system by running the command

```bash
docker version
```

If Docker is not on your system, follow the instructions [here][Docker] to install it.

If you're not running as root, ensure Docker can be run [without superuser privileges][docker-non-root].

If you're using Docker Desktop, ensure version 4.34 or higher is installed, and [host networking is enabled][docker-desktop-host-networking].

### Python

CLP uses Python to interpret the scripts that coordinate how it runs. Specifically, CLP needs Python version 3.8 or higher. Many Linux distributions come with Python pre-installed; to confirm that it's on your system, run the command

```bash
python3 --version
```

If Python isn't on your system, or if the version isn't high enough, install or upgrade it.

:::{note}
If you're planning to deploy CLP on multiple nodes/systems, there are a few other system requirements; check out the [multi-node deployment](../../guides/guides-multi-node/multi-node) page for more details.
:::

---

## Choosing a flavour

CLP has two flavours. Each flavour deals with a different type of log.

* **[clp-json](#clp-json)** for compressing and searching **JSON** logs.
* **[clp-text](#clp-text)** for compressing and searching **unstructured text** logs.

:::{note}
Both flavours contain the same binaries but are configured with different values for the
`package.storage_engine` key in the package's config file (`etc/clp-config.yml`).
:::

### clp-json

The JSON flavour of CLP is appropriate for JSON logs, where each log event is an independent JSON
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

If your logs are JSON logs, download and extract the JSON flavour tarball from the [Releases][clp-releases] page on GitHub, and proceed to the clp-json portal below.

::::{grid} 1 1 1 1
:gutter: 2

:::{grid-item-card}
:link: ../clp-json/main-page
CLP for JSON logs
^^^
Learn how to start up clp-json, and begin compressing and searching JSON logs.
:::
::::

### clp-text

The unstructured text flavour of CLP is appropriate for unstructured text logs, where each log event contains a
timestamp and may span one or more lines.

:::{note}
If your logs do not contain timestamps or CLP can't automatically parse the timestamps in your logs,
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
line while the second contains multiple lines.

If your logs are unstructured text logs, download and extract the unstructured text flavour tarball from the [Releases][clp-releases] page on GitHub, and proceed to the clp-text portal below.

::::{grid} 1 1 1 1
:gutter: 2

:::{grid-item-card}
:link: ../clp-text/main-page
CLP for unstructured logs
^^^
Learn how to start up clp-text, and begin compressing and searching unstructured text logs.
:::
::::

[Docker]: https://docs.docker.com/engine/install/
[docker-non-root]: https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user
[docker-desktop-host-networking]: https://docs.docker.com/engine/network/drivers/host/#docker-desktop
[clp-releases]: https://github.com/y-scope/clp/releases
