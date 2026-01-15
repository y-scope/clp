# clp-text quick-start

This page will walk you through how to start CLP and use it to compress and search unstructured
text logs.

:::{caution}
If you're using a `clp-text` release, you should only compress unstructured text logs. `clp-text`
is able to compress and search JSON logs as if they were unstructured text, but `clp-text` cannot
query individual fields. This limitation will be addressed in a future version of CLP.
:::

---

## Starting CLP

To start CLP, run:

```bash
sbin/start-clp.sh
```

:::{tip}
To validate configuration and prepare directories without launching services, add the
`--setup-only` flag (e.g., `sbin/start-clp.sh --setup-only`).
:::

:::{note}
If CLP fails to start (e.g., due to a port conflict), try adjusting the settings in
`etc/clp-config.yaml` and then run the start command again.
:::

:::{warning}
**Do not comment out or remove the `package` block in `etc/clp-config.yaml`**; otherwise, the storage
and query engines will default to `clp-s`, which is optimized for JSON logs rather than unstructured
text logs.

To use `clp-text`, the `package` block should be configured as follows:

```yaml
package:
  storage_engine: "clp"
  query_engine: "clp"
```
:::

---

## Compressing unstructured text logs

To compress some unstructured text logs, run:

```bash
sbin/compress.sh <path1> [<path2> ...]
```

`<path...>` are paths to unstructured text log files or directories containing such files.

The compression script will output the compression ratio of each dataset you compress, or you can
use the UI to view overall statistics.

Compressed logs will be stored in the directory specified by the `archive_output.storage.directory`
config option in `etc/clp-config.yaml` (`archive_output.storage.directory` defaults to
`var/data/archives`).

### Sample logs

For some sample logs, check out the [open-source datasets](../resources-datasets).

---

## Searching unstructured text logs

You can search your compressed logs from CLP's [UI](#searching-from-the-ui) or the
[command line](#searching-from-the-command-line).

In clp-text, queries are written as wildcard expressions. A wildcard expression is a plain text
query where:

* `*` matches zero or more characters
* `?` matches any single character

For example, consider the query in [Figure 1](#figure-1) and the logs in [Figure 2](#figure-2).

(figure-1)=
:::{card}

```bash
"INFO container_? Transitioned*ACQUIRED"
```

+++
**Figure 1**: An example query.
:::

(figure-2)=
:::{card}

```text
2015-03-23T15:50:17.926Z INFO container_1 Transitioned from ALLOCATED to ACQUIRED
2015-03-23T15:50:17.927Z ERROR Scheduler: Error trying to assign container token
java.lang.IllegalArgumentException: java.net.UnknownHostException: i-e5d112ea
    at org.apache.hadoop.security.buildTokenService(SecurityUtil.java:374)
    at org.apache.hadoop.ipc.Server$Handler.run(Server.java:2033)
Caused by: java.net.UnknownHostException: i-e5d112ea
    ... 17 more
```

+++
**Figure 2**: A set of unstructured text log events.
:::

The query in [Figure 1](#figure-1) will match with the first log message, as the `?` will match the
character "1", and the `*` will match the text " from ALLOCATED to ".

A complete reference for clp-text's query syntax is available on the
[syntax reference page](../reference-text-search-syntax).

### Searching from the UI

To search your compressed logs from CLP's UI, open [http://localhost:4000](http://localhost:4000) in
your browser (if you changed `webui.host` or `webui.port` in `etc/clp-config.yaml`, use the new
values).

[Figure 3](#figure-3) shows the search page after running a query.

(figure-3)=
:::{card}

:::{image} clp-text-search-ui.png

+++
**Figure 3**: The search page in CLP's UI.
:::

The numbered circles in [Figure 3](#figure-3) correspond to the following elements:

1. **The query input box**. The format of your query should conform to CLP's
   [unstructured text search syntax](../reference-text-search-syntax.md).
2. **The query case-sensitivity toggle**. When turned on, CLP will search for log events that match
   the case of your query.
3. **The time range selector**. CLP will search for log events that are in the specified time range.
   You can select a preset filter (e.g., `Last 15 minutes`; `Yesterday`) from the dropdown, or
   choose `Custom` and set the start time and end time directly.
4. **The search results timeline**. After a query, the timeline will show the number of results
   across the time range of your query.
   * You can click and drag to zoom into a time range.
   * When you mouse over a bar in the timeline, a popup will display the range and the number of
     search results in that range.
5. **The search results**.
   * You can sort by `Ascending` or `Descending` timestamp by clicking the `Timestamp` header in the
     table.
   * Each search result includes a link to the original file in which the log event was found.

:::{note}
By default, the UI will only return 1,000 of the latest search results. To perform searches which
return more results, use the [command line](#searching-from-the-command-line).
:::

### Searching from the command line

To search your compressed logs from the command line, run:

```bash
sbin/search.sh '<query>'
```

To narrow your search to a specific time range:

* Add `--begin-time <epoch-timestamp-millis>` to filter for log events after a certain time.
  * `<epoch-timestamp-millis>` is the timestamp as milliseconds since the UNIX epoch.
* Add `--end-time <epoch-timestamp-millis>` to filter for log events before a certain time.

To perform case-insensitive searches, add the `--ignore-case` flag.

:::{caution}
To match the convention of other tools, by default, searches are case-**insensitive** in the UI and
searches are case-**sensitive** on the command line.
:::

---

## Stopping CLP

If you need to stop CLP, run:

```bash
sbin/stop-clp.sh
```
