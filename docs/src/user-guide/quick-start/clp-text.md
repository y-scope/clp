# clp-text quick-start

This page will walk you through how to start up CLP, and then use it to compress and search unstructured text logs.

:::{caution}
If you're using the `clp-text` release, you should only compress unstructured text logs. `clp-text` is able to compress and search JSON logs as if it was unstructured text, but `clp-text` cannot query individual fields. This limitation will be addressed in a future version of CLP.
:::

---

```{include} ../../_shared/start-clp.md
```

---

## Compressing unstructured text logs

To compress unstructured text logs, run the following from inside the package directory:

```bash
sbin/compress.sh <path1> [<path2> ...]
```

`<path...>` are paths to unstructured text log files or directories containing such files.

Compressed logs will be stored in the `/var/bin/archives` directory.

```{include} ../../_shared/sample-logs.md
```

```{include} ../../_shared/compression-stats.md
```

---

## Searching unstructured text logs

You can search through your logs using queries from the UI or from the command line.

### Queries

Regardless of what method you use to search, you'll need a query to find the logs you're looking for. All unstructured text log queries are written as plain text. 

You can use a couple of special characters to make these queries more versatile. `*` can be used as a placeholder for an unknown number of characters, and `?` can be used for a single character. For example, the query

```bash
a*b?c
```

would return all logs that contain the character `"a"` followed by any number (including zero) of other characters, followed by `"b"`, followed by one other character, followed by `"c"`.

There are a number of other syntax rules specific to unstructured text queries that you can use to make your searches more powerful and effective. You can read about these syntax rules [here](../reference-text-search-syntax).

```{include} ../../_shared/search-options.md
```

---

```{include} ../../_shared/stop-clp.md
```

---

## More information

You've reached the end of the clp-text quick-start guide. For more information on clp-text, visit the [CLP for unstructured logs](../core-unstructured/index) page.
