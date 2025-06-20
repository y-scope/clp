# clp-json quick-start

This page will walk you through how to start up CLP and use it to compress and search JSON logs.

:::{caution}
If you're using the `clp-json` release, you can only compress JSON logs.
:::

---

```{include} ../../_shared/start-clp.md
```

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
  Log events without the specified timestamp key will _not_ have an assigned timestamp. Currently,
  these events can only be searched from the command line (when you don't specify a timestamp
  filter).
  :::

* `<path...>` are paths to JSON log files or directories containing such files.
    * Each JSON log file should contain each log event as a separate JSON object,
      i.e., _not_ as an array.

Compressed logs will be stored in the `/var/bin/archives` directory.

:::{tip}
To compress logs from object storage, see
[Using object storage](../guides-using-object-storage/index).
:::

```{include} ../../_shared/sample-logs.md
```

```{include} ../../_shared/compression-stats.md
```

---

## Searching JSON logs

You can search through your logs using queries from the UI or from the command line.

### Queries

Regardless of what method you use to search, you'll need a query to find the logs you're looking for. Queries for JSON logs take the general form of

```bash
key: value
```

where `key` is the name of the JSON key you'd like to search within, and `value` is a sequence of characters you're looking for within that key. Multiple key-value pairs can be chained together with `AND`, `OR`, or `NOT`, like so:

```bash
key1: value1 AND key2: value2 OR key3: value3 ...
```

There are a number of other JSON-specific syntax rules that you can use to make your searches more powerful and effective. You can read about these syntax rules [here](../reference-json-search-syntax).

```{include} ../../_shared/search-options.md
```

---

```{include} ../../_shared/stop-clp.md
```

---

## More information

You've reached the end of the clp-json quick-start guide. For more information on clp-json, visit the [CLP for JSON logs](../core-clp-s) page.
