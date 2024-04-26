:::{image} https://yscope.com/img/clp-logo.png
:class: bg-transparent
:width: 300
:::

YScope's Compressed Log Processor (CLP) compresses your logs, and allows you to search the
compressed logs without decompression. CLP supports both JSON logs and unstructured (i.e., free
text) logs. It also supports real-time log compression within several logging libraries. CLP also
includes purpose-built web interfaces for searching and viewing the compressed logs. To learn more
about it, you can read our [paper][1].

# Benchmarks

![CLP Benchmark on JSON Logs](_static/clp-json-benchmark.png)
![CLP Benchmark on Unstructured Logs](_static/clp-unstructured-benchmark.png)

The figures above show CLP's compression and search performance compared to other tools. We separate
the experiments between JSON and unstructured logs because (1) some tools can only handle one type
of logs, and (2) tools that can handle both types often have different designs for each type (such
as CLP).

Compression ratio is measured as the average across a variety of log datasets. Some of these
datasets can be found [here][2]. Search performance is measured using queries on the MongoDB logs
(for JSON) and the Hadoop logs (for unstructured logs). Note that CLP uses an index-less design, so
for a fair comparison, we disabled MongoDB and PostgreSQL's indexes; If we left them enabled,
MongoDB and PostgreSQL's compression ratio would be worse. We didn't disable indexing for
Elasticsearch or Splunk since these tools are fundamentally index-based (i.e., logs cannot be
searched without indexes). More details about our experimental methodology can be found in the
[CLP paper][1].

# System Overview

![CLP systems overview](_static/clp-complete-solution.png)

CLP provides an end-to-end log management pipeline consisting of compression, search, analytics, and
viewing. The figure above shows the CLP ecosystem architecture. It consists of the following
features:

- **Compression and Search**: CLP compresses logs into archives, which can be searched and analyzed
  in a [web UI][3]. The input can either be raw logs or CLP's compressed IR
  (intermediate representation) produced by CLP's logging libraries.

- **Real-time Compression with CLP Logging Libraries**: CLP provides logging libraries for
  [Python][4] and Java ([Log4j][5] and [Logback][6]). The logging libraries compress logs in
  real-time, so only compressed logs are written to disk or transmitted over the network. The
  compressed logs use CLP's intermediate representation (IR) format which achieves a higher
  compression ratio than general purpose compressors like Zstandard. Compressing IR into archives
  can further double the compression ratio and enable global search, but this requires more memory
  usage as it needs to buffer enough logs. More details on IR versus archives can be found in this
  [Uber Engineering Blog][7].

- **[Log Viewer][8]**: the compressed IR can be viewed in a web-based log viewer. Compared to
  viewing the logs in an editor, CLP's log viewer supports advanced features like filtering logs
  based on log level verbosity (e.g., only displaying logs with log level equal or higher than
  ERROR). These features are possible because CLP's logging libraries parse the logs before
  compressing them into IR.

- **IR Analytics Libraries**: we also provide a [Python library][9] and a [Go library][10] that can
  analyze compressed IR.

- **[Log parser][11]**: CLP also includes a custom pushdown-automata-based log parser that is 3x
  faster than state-of-the-art regular expression engines like [RE2][12]. The log parser is
  available as a library that can be used by other applications.

:::{toctree}
:hidden:

user-guide/index
dev-guide/index
:::

[1]: https://www.usenix.org/system/files/osdi21-rodrigues.pdf
[2]: user-guide/resources-datasets.md
[3]: https://github.com/y-scope/clp/blob/main/components/webui
[4]: https://github.com/y-scope/clp-loglib-py
[5]: https://github.com/y-scope/log4j1-appenders
[6]: https://github.com/y-scope/logback-appenders
[7]: https://www.uber.com/en-US/blog/reducing-logging-cost-by-two-orders-of-magnitude-using-clp
[8]: https://github.com/y-scope/yscope-log-viewer
[9]: https://github.com/y-scope/clp-ffi-py
[10]: https://github.com/y-scope/clp-ffi-go
[11]: https://github.com/y-scope/log-surgeon
[12]: https://github.com/google/re2
