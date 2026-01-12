# clp-text quick-start

This page will walk you through how to start CLP and use it to compress and search unstructured
text logs.

:::{caution}
If you're using a `clp-text` release, you should only compress unstructured text logs. `clp-text`
is able to compress and search JSON logs as if they were unstructured text, but `clp-text` cannot
query individual fields. This limitation will be addressed in a future version of CLP.
:::

---

## Starting CLP

::::{tab-set}
:::{tab-item} Docker Compose
:sync: docker

To start CLP, run:

```bash
sbin/start-clp.sh
```

```{tip}
To validate configuration and prepare directories without launching services, add the
`--setup-only` flag (e.g., `sbin/start-clp.sh --setup-only`).
```

```{note}
If CLP fails to start (e.g., due to a port conflict), try adjusting the settings in
`etc/clp-config.yaml` and then run the start command again.
```

````{warning}
**Do not comment out or remove the `package` block in `etc/clp-config.yaml`**; otherwise, the
storage and query engines will default to `clp-s`, which is optimized for JSON logs rather than
unstructured text logs.

To use `clp-text`, the `package` block should be configured as follows:

```yaml
package:
  storage_engine: "clp"
  query_engine: "clp"
```
````

For more details on Docker Compose deployment, see the
[Docker Compose deployment guide][docker-compose-deployment].
:::

:::{tab-item} Kubernetes (kind)
:sync: kind

First, create a kind cluster:

```bash
# Data and logs directory for the CLP Package
export CLP_HOME="$HOME/clp"

# Host port mappings
export CLP_WEBUI_PORT=30000
export CLP_RESULTS_CACHE_PORT=30017
export CLP_API_SERVER_PORT=30301
export CLP_DATABASE_PORT=30306
export CLP_MCP_SERVER_PORT=30800

# Credentials (generate random or use your own)
export CLP_DB_PASS=$(openssl rand -hex 16)
export CLP_DB_ROOT_PASS=$(openssl rand -hex 16)
export CLP_QUEUE_PASS=$(openssl rand -hex 16)
export CLP_REDIS_PASS=$(openssl rand -hex 16)

# Create required directories
mkdir -p "$CLP_HOME/var/"{data,log}/{database,queue,redis,results_cache} \
         "$CLP_HOME/var/data/"{archives,streams,staged-archives,staged-streams} \
         "$CLP_HOME/var/log/"{compression_scheduler,compression_worker,user} \
         "$CLP_HOME/var/log/"{query_scheduler,query_worker,reducer} \
         "$CLP_HOME/var/log/"{garbage_collector,api_server,log_ingestor,mcp_server} \
         "$CLP_HOME/var/tmp"

# Create the kind cluster
cat <<EOF | kind create cluster --name clp --config=-
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
- role: control-plane
  extraMounts:
  - hostPath: $CLP_HOME
    containerPath: $CLP_HOME

  # Mount for logs input (change the paths as needed; not needed if using S3 input)
  - hostPath: /home
    containerPath: /home

  extraPortMappings:
  - containerPort: $CLP_WEBUI_PORT
    hostPort: $CLP_WEBUI_PORT
  - containerPort: $CLP_RESULTS_CACHE_PORT
    hostPort: $CLP_RESULTS_CACHE_PORT
  - containerPort: $CLP_API_SERVER_PORT
    hostPort: $CLP_API_SERVER_PORT
  - containerPort: $CLP_DATABASE_PORT
    hostPort: $CLP_DATABASE_PORT
  - containerPort: $CLP_MCP_SERVER_PORT
    hostPort: $CLP_MCP_SERVER_PORT
EOF
```

Then, install the Helm chart with clp-text configuration:

```bash
cd tools/deployment/package-helm
helm install clp . \
  --set clpConfig.package.storage_engine=clp \
  --set clpConfig.package.query_engine=clp \
  --set clpConfig.webui.port="$CLP_WEBUI_PORT" \
  --set clpConfig.results_cache.port="$CLP_RESULTS_CACHE_PORT" \
  --set clpConfig.api_server.port="$CLP_API_SERVER_PORT" \
  --set clpConfig.database.port="$CLP_DATABASE_PORT" \
  --set clpConfig.mcp_server.port="$CLP_MCP_SERVER_PORT" \
  --set clpConfig.data_directory="$CLP_HOME/var/data" \
  --set clpConfig.logs_directory="$CLP_HOME/var/log" \
  --set clpConfig.tmp_directory="$CLP_HOME/var/tmp" \
  --set clpConfig.archive_output.storage.directory="$CLP_HOME/var/data/archives" \
  --set clpConfig.stream_output.storage.directory="$CLP_HOME/var/data/streams" \
  --set credentials.database.password="$CLP_DB_PASS" \
  --set credentials.database.root_password="$CLP_DB_ROOT_PASS" \
  --set credentials.queue.password="$CLP_QUEUE_PASS" \
  --set credentials.redis.password="$CLP_REDIS_PASS"
```

```{note}
To use sbin scripts with this deployment, do not set `allowHostAccessForSbinScripts` to `false`
(it is `true` by default).
```

Wait for all pods to be ready:

```bash
kubectl wait pods --all --for=condition=Ready --timeout=300s
```

For more details on Kubernetes deployment, see the [Kubernetes deployment guide][k8s-deployment].
:::
::::

---

## Compressing unstructured text logs

::::{tab-set}
:::{tab-item} Docker Compose
:sync: docker

No additional configuration is required.
:::

:::{tab-item} Kubernetes (kind)
:sync: kind

Configure `etc/clp-config.yaml` to connect to the kind-deployed database:

```yaml
database:
  port: 30306
```

:::
::::

To compress some unstructured text logs, run:

```bash
sbin/compress.sh <path1> [<path2> ...]
```

`<path...>` are paths to unstructured text log files or directories containing such files.

The compression script will output the compression ratio of each dataset you compress, or you can
use the UI to view overall statistics.

Compressed logs will be stored in the directory specified by the `archive_output.storage.directory`
config option in `etc/clp-config.yaml` (`archive_output.storage.directory` defaults to
`var/data/archives`).

### Sample logs

For some sample logs, check out the [open-source datasets][datasets].

---

## Searching unstructured text logs

You can search your compressed logs from CLP's [UI](#searching-from-the-ui) or the
[command line](#searching-from-the-command-line).

In clp-text, queries are written as wildcard expressions. A wildcard expression is a plain text
query where:

* `*` matches zero or more characters
* `?` matches any single character

For example, consider the query in [Figure 1](#figure-1) and the logs in [Figure 2](#figure-2).

(figure-1)=
:::{card}

```bash
"INFO container_? Transitioned*ACQUIRED"
```

+++
**Figure 1**: An example query.
:::

(figure-2)=
:::{card}

```text
2015-03-23T15:50:17.926Z INFO container_1 Transitioned from ALLOCATED to ACQUIRED
2015-03-23T15:50:17.927Z ERROR Scheduler: Error trying to assign container token
java.lang.IllegalArgumentException: java.net.UnknownHostException: i-e5d112ea
    at org.apache.hadoop.security.buildTokenService(SecurityUtil.java:374)
    at org.apache.hadoop.ipc.Server$Handler.run(Server.java:2033)
Caused by: java.net.UnknownHostException: i-e5d112ea
    ... 17 more
```

+++
**Figure 2**: A set of unstructured text log events.
:::

The query in [Figure 1](#figure-1) will match with the first log message, as the `?` will match the
character "1", and the `*` will match the text " from ALLOCATED to ".

A complete reference for clp-text's query syntax is available on the
[syntax reference page][text-search-syntax].

### Searching from the UI

To search your compressed logs from CLP's UI, open [http://localhost:4000](http://localhost:4000)
(Docker Compose) or [http://localhost:30000](http://localhost:30000) (Kubernetes) in your browser.

:::{note}
If you changed `webui.host` or `webui.port` in the configuration, use the new values.
:::

[Figure 3](#figure-3) shows the search page after running a query.

(figure-3)=
:::{card}

:::{image} clp-text-search-ui.png

+++
**Figure 3**: The search page in CLP's UI.
:::

The numbered circles in [Figure 3](#figure-3) correspond to the following elements:

1. **The query input box**. The format of your query should conform to CLP's
   [unstructured text search syntax][text-search-syntax].
2. **The query case-sensitivity toggle**. When turned on, CLP will search for log events that match
   the case of your query.
3. **The time range selector**. CLP will search for log events that are in the specified time range.
   You can select a preset filter (e.g., `Last 15 minutes`; `Yesterday`) from the dropdown, or
   choose `Custom` and set the start time and end time directly.
4. **The search results timeline**. After a query, the timeline will show the number of results
   across the time range of your query.
   * You can click and drag to zoom into a time range.
   * When you mouse over a bar in the timeline, a popup will display the range and the number of
     search results in that range.
5. **The search results**.
   * You can sort by `Ascending` or `Descending` timestamp by clicking the `Timestamp` header in the
     table.
   * Each search result includes a link to the original file in which the log event was found.

:::{note}
By default, the UI will only return 1,000 of the latest search results. To perform searches which
return more results, use the [command line](#searching-from-the-command-line) or
[API server](#searching-via-the-api-server).
:::

### Searching via the API server

To search via the API server:

```bash
curl -X POST "http://localhost:30301/query/submit" \
  -H "Content-Type: application/json" \
  -d '{
    "query_string": "<query>",
    "max_num_results": 1000,
    "timestamp_begin": null,
    "timestamp_end": null,
    "case_sensitive": false
  }'
```

For more details on the API, see [Using the API server][api-server].

### Searching from the command line

::::{tab-set}
:::{tab-item} Docker Compose
:sync: docker

No additional configuration is required.
:::

:::{tab-item} Kubernetes (kind)
:sync: kind

Configure `etc/clp-config.yaml` to connect to the kind-deployed services:

```yaml
database:
  port: 30306
results_cache:
  port: 30017
```

:::
::::

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

::::{tab-set}
:::{tab-item} Docker Compose
:sync: docker

If you need to stop CLP, run:

```bash
sbin/stop-clp.sh
```

:::

:::{tab-item} Kubernetes (kind)
:sync: kind

To stop CLP, uninstall the Helm release:

```bash
helm uninstall clp
```

To also delete the kind cluster:

```bash
kind delete cluster --name clp
```

:::
::::

[api-server]: ../guides-using-the-api-server.md
[datasets]: ../resources-datasets
[docker-compose-deployment]: ../guides-docker-compose-deployment.md
[k8s-deployment]: ../guides-k8s-deployment.md
[text-search-syntax]: ../reference-text-search-syntax.md
