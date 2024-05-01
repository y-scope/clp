# Searching from the UI

CLP includes a web interface available at [http://localhost:4000](http://localhost:4000) by default
(if you changed `webui.host` or `webui.port` in `etc/clp-config.yml`, use the new values). 

:::{image} ../../_static/clp-search-ui.png
:::

The image above shows the search page after running a query. The numbered circles correspond to the
following features:

1. The search box is where you can enter a query.
   * The format of a query depends on the format of your logs:
     [JSON](../reference-json-search-syntax.md) or [text](../reference-text-search-syntax.md).
2. The timeline shows the number of results across the time range of your query.
   * You can click and drag to zoom into a time range or use the time range filter in (4).
3. The table displays the search results for your query.
4. Clicking the <i class="fa fa-bars"></i> icon reveals additional filters for your query.
   * The time range filter allows you to period of time that matching log events must be in.
   * The case sensitivity filter allows you to specify whether CLP should respect the case of your
     query.
5. Clicking the <i class="fa fa-cog"></i> icon reveals options for displaying results.
6. The <i class="fa fa-trash"></i> icon clears the results of the last query.

:::{note}
By default, the UI will only return 1,000 of the latest search results. To perform searches which
return more results, use the [command line](cli-search.md).
:::