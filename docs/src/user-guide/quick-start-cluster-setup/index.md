# Cluster setup

To set up a cluster, you'll need to:

* Choose a release.
* Choose between a single or multi-node deployment.
* Ensure you meet the requirements for running the release.
* Configure the release (if necessary).
* Start CLP.

## Choosing a release

There are two flavours of CLP [releases][clp-releases]:

* **[clp-json](#clp-json)** for compressing and searching **JSON** logs.
* **[clp-text](#clp-text)** for compressing and searching **free-text** logs.

:::{note}
Both flavours contain the same binaries but are configured with different values for the
`package.storage_engine` key.
:::

You should download and extract the release that's appropriate for your logs.

(clp-json)=
### clp-json

`clp-json` releases are appropriate for JSON logs where each log event is an independent JSON
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

(clp-text)=
### clp-text

`clp-text` releases are appropriate for free text logs where each log event contains a timestamp
and may span one or more lines.

:::{note}
If your logs do not contain timestamps or CLP can't automatically parse the timestamps in your logs,
it will treat each line as an independent log event.
:::

For example:

```
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

## Deployment options

Choose one of the deployment options below:

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: single-node
Single-node deployment
:::

:::{grid-item-card}
:link: multi-node
Multi-node deployment
:::
::::

:::{toctree}
:hidden:
:caption: Cluster setup

single-node
multi-node
:::

[clp-releases]: https://github.com/y-scope/clp/releases
