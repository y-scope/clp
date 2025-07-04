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

:::{note}
If CLP fails to start (e.g., due to a port conflict), try adjusting the settings in
`etc/clp-config.yml` and then run the start command again.
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
config option in `etc/clp-config.yml` (`archive_output.storage.directory` defaults to
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
your browser (if you changed `webui.host` or `webui.port` in `etc/clp-config.yml`, use the new
values).

:::{image} clp-search-ui.png
:::

The image above shows the search page after running a query. The numbered circles correspond to the
following features:

1. The search box is where you can enter your query.
2. The timeline shows the number of results across the time range of your query.
   * You can click and drag to zoom into a time range, or use the time range filter in (4).
3. The table displays the search results for your query.
4. Clicking the <i class="fa fa-bars"></i> icon reveals additional filters for your query.
   * The time range filter allows you to specify the period of time that matching log events must be
     in.
   * The case sensitivity filter allows you to specify whether CLP should respect the case of your
     query.
5. Clicking the <i class="fa fa-cog"></i> icon reveals options for displaying results.
6. The <i class="fa fa-trash"></i> icon clears the results of the last query.

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
