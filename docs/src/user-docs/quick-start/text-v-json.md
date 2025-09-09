# clp-text vs. clp-json

CLP comes in two flavors:

* **[clp-json](#clp-json)** for compressing and searching **JSON** logs.
* **[clp-text](#clp-text)** for compressing and searching **unstructured text** logs.

:::{note}
Both flavors contain the same binaries but are configured with different values for the
`package.storage_engine` key in the package's config file (`etc/clp-config.yml`).
:::

[Table 1](#table-1) compares the different capabilities and limitations of each of the two flavors.

(table-1)=
:::{card}
<style>
.g,.r,.o{font-weight:700;font-style:normal}
.g::after{content:"✓";color:green}
.r::after{content:"✗";color:red}
.o::after{content:"〇";color:orange}
</style>

|Capability|`clp-text`|`clp-json`|
|---|:---:|:---:|
|Compression of unstructured text logs|<b class="g"></b>|<b class="r"></b>|
|Compression of JSON logs|<b class="o"></b><sup>1</sup>|<b class="g"></b>|
|Compression of CLP IR files|<b class="g"></b>|<b class="r"></b>|
|Compression of CLP KV-IR files|<b class="r"></b>|<b class="r"></b>|
|Command line search|<b class="g"></b>|<b class="g"></b>|
|WebUI search|<b class="g"></b>|<b class="g"></b>|
|Decompression|<b class="g"></b>|<b class="r"></b>|
|Automatic timestamp parsing|<b class="o"></b><sup>2</sup>|<b class="o"></b><sup>2, 3</sup>|
|Preservation of time zone information|<b class="r"></b><sup>4</sup>|<b class="r"></b><sup>4</sup>|
|Retention control|<b class="g"></b>|<b class="g"></b>|
|Archive management|<b class="g"></b>|<b class="g"></b>|
|Dataset management|<b class="r"></b>|<b class="g"></b>|
|S3 support|<b class="r"></b>|<b class="g"></b>|
|Multi-node deployment|<b class="g"></b>|<b class="g"></b>|
|CLP + Presto integration|<b class="r"></b>|<b class="g"></b>|
|Parallel compression|<b class="g"></b>|<b class="g"></b>|

+++
**Table 1**: The capabilities and limitations of CLP's two flavors.

1) `clp-text` is able to compress and search JSON logs as if they were unstructured text, but
   `clp-text` cannot query individual fields.
2) Timestamp parsing is limited to specific supported formats: see
   [clp-text timestamp formats][ts-text] and [clp-json timestamp formats][ts-json] for more details.
3) Timestamps are parsed automatically as long as the timestamp key for the logs is provided at
   compression time using the `--timestamp-key` flag.
4) We hope to introduce support for the preservation of time zone information in a future update
   (issue is up [here](https://github.com/y-scope/clp/issues/1290))
:::

## clp-json

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

## clp-text

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

[clp-releases]: https://github.com/y-scope/clp/releases
<!-- markdownlint-disable-next-line MD013 -->
[ts-text]: https://github.com/y-scope/clp/blob/main/components/core/src/clp/TimestampPattern.cpp#L120
<!-- markdownlint-disable-next-line MD013 -->
[ts-json]: https://github.com/y-scope/clp/blob/main/components/core/src/clp_s/TimestampPattern.cpp#L210
