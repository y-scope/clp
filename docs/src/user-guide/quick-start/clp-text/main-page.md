# CLP-text

This page will walk you through how to start up CLP, and also show you how to use CLP to compress and search unstructured text logs.

:::{caution}
If you're using the `clp-text` release, you should only compress unstructured text logs. `clp-text` can compress and search JSON logs as if it was unstructured text, but `clp-text` cannot query individual fields. This limitation will be addressed in a future version of CLP.
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

## Compressing unstructured text logs

To compress unstructured text logs, run the following from inside the package directory:

```bash
sbin/compress.sh <path1> [<path2> ...]
```

`<path...>` are paths to unstructured text log files or directories containing such files.

Compressed logs will be stored in the `/var/bin/archives` directory.

# Sample logs

For some sample logs, check out the open-source [datasets](../../resources-datasets.md).

# Examining compression statistics

The compression script used above will output the compression ratio of each dataset you compress, or
you can use the UI to view overall statistics.

---

## Searching unstructured text logs

You can search through your logs using queries from the UI or from the command line.

### Queries

Regardless of what method you use to search, you'll need a query to find the logs you're looking for. All unstructured text log queries are written as plain text. 

You can use a couple special characters to make these queries more versatile. `*` can be used as a placeholder for an unknown number of characters, and `?` can be used for a single character. For example, the query

``` bash
one * two?three
```

would return all logs that contain the sequence of characters "one" followed by any number (even zero) of other characters, followed by "two", followed by one other character, followed by "three".




There are a number of other syntax rules specific to unstructured text queries that you can use to make your searches more powerful and effective. You can read about these syntax rules [here](../../reference-text-search-syntax).

### Searching from the command line

If you'd like to search your query from the command line, run the following command from inside the package:

```bash
sbin/search.sh '<query>'
```

To narrow your search to a specific time range:

* Add `--begin-time <epoch-timestamp-millis>` to filter for log events after a certain time.
    * `<epoch-timestamp-millis>` is the timestamp as milliseconds since the UNIX epoch.
* Add `--end-time <epoch-timestamp-millis>` to filter for log events after a certain time.

To perform case-insensitive searches, add the `--ignore-case` flag.

:::{caution}
To match the convention of other tools, by default, searches are case-**insensitive** in the UI and
searches are case-**sensitive** on the command line.
:::

### Searching from the UI

If you'd like to search your query from the web UI, CLP includes a web interface available at [http://localhost:4000](http://localhost:4000) by default (if you changed `webui.host` or `webui.port` in `etc/clp-config.yml`, use the new values).

:::{image} clp-search-ui.png
:::

The image above shows the search page after running a query. The numbered circles correspond to the following features:

1. The search box is where you can enter your query.
2. The timeline shows the number of results across the time range of your query.
   * You can click and drag to zoom into a time range or use the time range filter in (4).
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
