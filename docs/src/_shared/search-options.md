---
orphan: true        # keep Sphinx quiet about the ToC
---

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

:::{image} ../../clp-search-ui.png
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
