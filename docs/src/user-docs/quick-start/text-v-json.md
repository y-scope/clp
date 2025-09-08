# `clp-text` vs. `clp-json`

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

| Capability                            | `clp-text`                            | `clp-json`                            |
| ------------------------------------- | :-----------------------------------: | :-----------------------------------: |
| Compression of unstructured text logs | <span style="color: green"><strong>✓</strong></span> | <span style="color: red"><strong>✗</strong></span> |
| Compression of JSON logs              | <span style="color: orange"><strong>〇</strong><sup>1</sup></span> | <span style="color: green"><strong>✓</strong></span> |
| Compression of CLP IR files           | <span style="color: green"><strong>✓</strong></span> | <span style="color: red"><strong>✗</strong></span> |
| Compression of CLP KV-IR files        | <span style="color: red"><strong>✗</strong></span> | <span style="color: red"><strong>✗</strong></span> |
| Command line search                   | <span style="color: green"><strong>✓</strong></span> | <span style="color: green"><strong>✓</strong></span> |
| WebUI search                          | <span style="color: green"><strong>✓</strong></span> | <span style="color: green"><strong>✓</strong></span> |
| Decompression                         | <span style="color: green"><strong>✓</strong></span> | <span style="color: red"><strong>✗</strong></span> |
| Automatic timestamp parsing           | <span style="color: orange"><strong>〇</strong><sup>2</sup></span> | <span style="color: orange"><strong>〇</strong><sup>2,3</sup></span> |
| Preservation of time zone information | <span style="color: red"><strong>✗</strong><sup>4</sup></span> | <span style="color: red"><strong>✗</strong><sup>4</sup></span> |
| Retention control                     | <span style="color: green"><strong>✓</strong></span> | <span style="color: green"><strong>✓</strong></span> |
| Archive management                    | <span style="color: green"><strong>✓</strong></span> | <span style="color: green"><strong>✓</strong></span> |
| Dataset management                    | <span style="color: red"><strong>✗</strong></span> | <span style="color: green"><strong>✓</strong></span> |
| S3 support                            | <span style="color: red"><strong>✗</strong></span> | <span style="color: green"><strong>✓</strong></span> |
| Multi-node deployment                 | <span style="color: green"><strong>✓</strong></span> | <span style="color: green"><strong>✓</strong></span> |
| CLP + Presto integration              | <span style="color: red"><strong>✗</strong></span> | <span style="color: green"><strong>✓</strong></span> |
| Parallel compression                  | <span style="color: green"><strong>✓</strong></span> | <span style="color: green"><strong>✓</strong></span> |

+++
**Table 1**: The high-level schema of CLP's datasets table.
1) `clp-text` is able to compress and search JSON logs as if they were unstructured text, but
   `clp-text` cannot query individual fields.  
2) Timestamp parsing is limited to specific supported formats: see
   [clp-text timestamp formats](http://github.com/y-scope/clp/blob/bfd4f60ffe9c5d69618cc8416ec6729c76ee9862/components/core/src/clp/TimestampPattern.cpp#L120)
   and
   [clp-json timestamp formats](https://github.com/y-scope/clp/blob/bfd4f60ffe9c5d69618cc8416ec6729c76ee9862/components/core/src/clp_s/TimestampPattern.cpp#L210)
   for more details.  
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
