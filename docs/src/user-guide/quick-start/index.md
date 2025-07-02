# Overview

This guide describes the following:

* [CLP's system requirements](#system-requirements)
* [How to choose a CLP flavor](#choosing-a-flavor)
* [How to use CLP](#using-clp).

---

## System Requirements

To run a CLP release, you'll need:

* [Docker](#docker)
* [Python](#python)

### Docker

To check whether Docker is installed on your system, run:

```bash
docker version
```

If Docker isn't installed, follow [these instructions][Docker] to install it.

NOTE:

* If you're not running as root, ensure Docker can be run
  [without superuser privileges][docker-non-root].
* If you're using Docker Desktop, ensure version 4.34 or higher is installed, and
  [host networking is enabled][docker-desktop-host-networking].

### Python

To check whether Python is installed on your system, run:

```bash
python3 --version
```

CLP requires Python 3.8 or higher. If Python isn't installed, or if the version isn't high enough,
install or upgrade it by following the instructions for your OS.

---

## Choosing a flavor

There are two flavors of CLP:

* **[clp-json](#clp-json)** for compressing and searching **JSON** logs.
* **[clp-text](#clp-text)** for compressing and searching **unstructured text** logs.

:::{note}
Both flavors contain the same binaries but are configured with different values for the
`package.storage_engine` key in the package's config file (`etc/clp-config.yml`).
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
[Releases][clp-releases] page, then proceed to the [clp-json quick-start](./clp-json.md) guide.

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
[Releases][clp-releases] page, then proceed to the [clp-text quick-start](./clp-text.md) guide.

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

[clp-releases]: https://github.com/y-scope/clp/releases
[Docker]: https://docs.docker.com/engine/install/
[docker-desktop-host-networking]: https://docs.docker.com/engine/network/drivers/host/#docker-desktop
[docker-non-root]: https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user
