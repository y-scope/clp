# clp-json quick-start

This page will walk you through how to start CLP and use it to compress and search JSON logs.

:::{caution}
If you're using a `clp-json` release, you can only compress and search JSON logs. This limitation
will be addressed in a future version of CLP.
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

## Compressing JSON logs

To compress some JSON logs, run:

```bash
sbin/compress.sh --timestamp-key '<timestamp-key>' <path1> [<path2> ...]
```

* `<timestamp-key>` is the field path of the kv-pair that contains the timestamp in each log event.
  * E.g., if your log events look like
    `{"timestamp": {"iso8601": "2024-01-01 00:01:02.345", ...}}`, you should enter
    `timestamp.iso8601` as the timestamp key.

  :::{caution}
  Log events without the specified timestamp key will *not* have an assigned timestamp. Currently,
  these events can only be searched from the command line (when you don't specify a timestamp
  filter).
  :::

* `<path...>` are paths to JSON log files or directories containing such files.
  * Each JSON log file should contain each log event as a
    [separate JSON object](./index.md#clp-json), i.e., *not* as an array.

The compression script will output the compression ratio of each dataset you compress, or you can
use the UI to view overall statistics.

Compressed logs will be stored in the directory specified by the `archive_output.storage.directory`
config option in `etc/clp-config.yml` (`archive_output.storage.directory` defaults to
`var/data/archives`).

:::{tip}
To compress logs from object storage, see
[Using object storage](../guides-using-object-storage/index).
:::

### Sample logs

For some sample logs, check out the [open-source datasets](../resources-datasets).

---

## Searching JSON logs

You can search your compressed logs from CLP's [UI](#searching-from-the-ui) or the
[command line](#searching-from-the-command-line).

In clp-json, queries are written as a set of conditions (predicates) on key-value pairs (kv-pairs).
For example, [Figure 1](#figure-1) shows a query that matches the first log event in
[Figure 2](#figure-2).

(figure-1)=
:::{card}

```sql
ctx: "conn11" AND msg: "*write concern*"
```

+++
**Figure 1**: An example query.
:::

(figure-2)=
:::{card}

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

+++
**Figure 2**: A set of JSON log events.
:::

The query in [Figure 1](#figure-1) will match log events that contain the kv-pair `"ctx": "conn11"`
as well as a kv-pair with key `"msg"` and a value that matches the wildcard query
`"*write concern*"`.

A complete reference for clp-json's query syntax is available on the
[syntax reference page](../reference-json-search-syntax).

### Searching from the UI

To search your compressed logs from CLP's UI, open [http://localhost:4000](http://localhost:4000) in
your browser (if you changed `webui.host` or `webui.port` in `etc/clp-config.yml`, use the new
values).

:::{image} clp-search-ui.png
:::

The image above shows the search page after running a query. The numbered circles correspond to
the following features:

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
