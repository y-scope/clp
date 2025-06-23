# clp-json quick-start

This page will walk you through how to start up CLP and use it to compress and search JSON logs.

:::{caution}
If you're using the `clp-json` flavor, you can only compress JSON logs.
:::

---

## Starting CLP

To start CLP, run

```bash
sbin/start-clp.sh
```

:::{note}
If CLP fails to start (e.g., due to a port conflict), try adjusting the settings in
`etc/clp-config.yml` and then run the start command again.
:::

---

## Compressing JSON logs

To compress JSON logs, from inside the package directory, run:

```bash
sbin/compress.sh fs --timestamp-key '<timestamp-key>' <path1> [<path2> ...]
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
  * Each JSON log file should contain each log event as a separate JSON object,
    i.e., *not* as an array.

Compressed logs will be stored in the `/var/bin/archives` directory.

:::{tip}
To compress logs from object storage, see
[Using object storage](../guides-using-object-storage/index).
:::

### Sample logs

For some sample logs, check out the open-source [datasets](../resources-datasets).

### Examining compression statistics

The compression script used above will output the compression ratio of each dataset you compress,
or you can use the UI to view overall statistics.

---

## Searching JSON logs

You can search through your logs using queries from the UI or from the command line.

### Queries

Regardless of what method you use to search, you'll need a query to find the logs you're looking
for. Queries for JSON logs take the general form of

```bash
key: value
```

where `key` is the name of the JSON key you'd like to search within, and `value` is a sequence of
characters you're looking for within that key. Multiple key-value pairs can be chained together
with `AND`, `OR`, or `NOT`, like so:

```bash
key1: value1 AND key2: value2 OR key3: value3 ...
```

There are a number of other JSON-specific syntax rules that you can use to make your searches more
powerful and effective. You can read about these rules on the
[JSON syntax reference page](../reference-json-search-syntax).

### Searching from the command line

If you'd like to search your query from the command line, run the following command from inside the
package:

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

### Searching from the UI

If you'd like to search your query from the web UI, CLP includes a web interface available at
[http://localhost:4000](http://localhost:4000) by default (if you changed `webui.host` or
`webui.port` in `etc/clp-config.yml`, use the new values).

:::{image} ../../clp-search-ui.png
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

---

## Stopping CLP

If you need to stop CLP, run the command

```bash
sbin/stop-clp.sh
```

---

## More information

You've reached the end of the clp-json quick-start guide. For more information on clp-json,
visit the [CLP for JSON logs](../core-clp-s) page.
