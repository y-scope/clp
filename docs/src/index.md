:::{image} clp-logo.png
:class: bg-transparent
:width: 300
:::

YScope's Compressed Log Processor (CLP) compresses your logs, and allows you to search the
compressed logs without decompression. CLP supports both JSON logs and unstructured (i.e., free
text) logs. It also supports real-time log compression within several logging libraries. CLP also
includes purpose-built web interfaces for searching and viewing the compressed logs.

# Benchmarks

:::{image} clp-json-benchmark.png
:alt: CLP Benchmark on JSON Logs
:::

:::{image} clp-unstructured-benchmark.png
:alt: CLP Benchmark on Unstructured Logs
:::

The figures above show CLP's compression and search performance compared to other tools. We separate
the experiments between JSON and unstructured logs because (1) some tools can only handle one type
of logs, and (2) tools that can handle both types often have different designs for each type (such
as CLP).

Compression ratio is measured as the average across a variety of log datasets. Some of these
datasets can be found [here][datasets]. Search performance is measured using queries on the MongoDB
logs (for JSON) and the Hadoop logs (for unstructured logs). Note that CLP uses an index-less
design, so for a fair comparison, we disabled MongoDB and PostgreSQL's indexes; If we left them
enabled, MongoDB and PostgreSQL's compression ratio would be worse. We didn't disable indexing for
Elasticsearch or Splunk since these tools are fundamentally index-based (i.e., logs cannot be
searched without indexes). More details about our experimental methodology can be found in the
[CLP paper][clp-paper].

# System Overview

:::{image} clp-complete-solution.png
:alt: CLP systems overview
:::

CLP provides an end-to-end log management pipeline consisting of compression, search, analytics, and
viewing. The figure above shows the CLP ecosystem architecture. It consists of the following
features:

- **Compression and Search**: CLP compresses logs into archives, which can be searched and analyzed
  in a [web UI][webui]. The input can either be raw logs or CLP's compressed IR
  (intermediate representation) produced by CLP's logging libraries.

- **Real-time Compression with CLP Logging Libraries**: CLP provides logging libraries for
  [Python][clp-loglib-py] and Java ([Log4j1][log4j1-appenders], [Log4j2][log4j2-appenders], and 
  [Logback][logback-appenders]). The logging libraries compress logs in real-time, so only 
  compressed logs are written to disk or transmitted over the network. The compressed logs use CLP's
  intermediate representation (IR) format which achieves a higher compression ratio than general
  purpose compressors like Zstandard. Compressing IR into archives can further double the
  compression ratio and enable global search, but this requires more memory usage as it needs to
  buffer enough logs. More details on IR versus archives can be found in this
  [Uber Engineering Blog][uber-blog].

- **[Log Viewer][log-viewer]**: the compressed IR can be viewed in a web-based log viewer. Compared
  to viewing the logs in an editor, CLP's log viewer supports advanced features like filtering logs
  based on log level verbosity (e.g., only displaying logs with log level equal or higher than
  ERROR). These features are possible because CLP's logging libraries parse the logs before
  compressing them into IR.

- **IR Analytics Libraries**: we also provide a [Python library][clp-ffi-py] and a
  [Go library][clp-ffi-go] that can analyze compressed IR.

- **[Log parser][log-surgeon]**: CLP also includes a custom pushdown-automata-based log parser that
  is 3x faster than state-of-the-art regular expression engines like [RE2][re2]. The log parser is
  available as a library that can be used by other applications.

# Getting started

Check out the relevant docs below, based on whether you'd like to use or develop CLP.

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: user-docs/index
User docs
^^^
Docs for those interested in using and operating CLP.
:::

:::{grid-item-card}
:link: dev-docs/index
Developer docs
^^^
Docs for those interested in developing CLP.
:::
::::

# Getting in touch

## GitHub

You can use GitHub issues to [report a bug][bug-report] or [request a feature][feature-req].

## Community

Need help? Join us on one of our community servers:

* [![Discord][badge-discord]][yscope-community-discord]
* [![Slack][badge-slack]][yscope-community-slack]
* [![CLP on Zulip][badge-zulip]][yscope-community-zulip]

:::{toctree}
:hidden:

user-docs/index
dev-docs/index
:::

[badge-discord]: https://img.shields.io/discord/1377353873068392580?style=flat&logo=discord&logoColor=white&label=Discord&labelColor=%235561f5
[badge-slack]: https://img.shields.io/badge/Slack-yscope--community-1e724f?style=flat&logo=slack&logoColor=white&labelColor=4A154B
[badge-zulip]: https://img.shields.io/badge/Zulip-yscope--clp-1888FA?logo=zulip
[bug-report]: https://github.com/y-scope/clp/issues/new?assignees=&labels=bug&template=bug-report.yaml
[clp-ffi-go]: https://github.com/y-scope/clp-ffi-go
[clp-ffi-py]: https://github.com/y-scope/clp-ffi-py
[clp-loglib-py]: https://github.com/y-scope/clp-loglib-py
[clp-paper]: https://www.usenix.org/system/files/osdi21-rodrigues.pdf
[datasets]: user-docs/resources-datasets
[feature-req]: https://github.com/y-scope/clp/issues/new?assignees=&labels=enhancement&template=feature-request.yaml
[log-surgeon]: https://github.com/y-scope/log-surgeon
[log-viewer]: https://github.com/y-scope/yscope-log-viewer
[log4j1-appenders]: https://github.com/y-scope/log4j1-appenders
[log4j2-appenders]: https://github.com/y-scope/log4j2-appenders
[logback-appenders]: https://github.com/y-scope/logback-appenders
[re2]: https://github.com/google/re2
[uber-blog]: https://www.uber.com/en-US/blog/reducing-logging-cost-by-two-orders-of-magnitude-using-clp
[webui]: https://github.com/y-scope/clp/tree/DOCS_VAR_CLP_GIT_REF/components/webui
[yscope-community-discord]: https://discord.gg/7kZA2m5G87
[yscope-community-slack]: https://communityinviter.com/apps/yscopecommunity/yscope-community
[yscope-community-zulip]: https://yscope-clp.zulipchat.com
