# Searching from the command line

From inside the package, run:

```
sbin/search.sh '<query>'
```

The format of `<query>` depends on the format your logs: [JSON](../reference-json-search-syntax.md)
or [text](../reference-text-search-syntax.md).

To narrow your search to a specific time range:

* Add `--begin-time <epoch-timestamp-millis>` to filter for log events after a certain time.
    * `<epoch-timestamp-millis>` is the timestamp as milliseconds since the UNIX epoch.
* Add `--end-time <epoch-timestamp-millis>` to filter for log events after a certain time.

To perform case-insensitive searches, add the `--ignore-case` flag.

:::{caution}
To match the convention of other tools, by default, searches are case-**insensitive** in the UI and
searches are case-**sensitive** on the command line.
:::
