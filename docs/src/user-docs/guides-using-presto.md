# Using Presto with CLP

[Presto] is a distributed SQL query engine that can be used to query data stored in CLP (using SQL).
This guide describes how to set up and use Presto with CLP.

:::{warning}
Currently, only the [clp-json](quick-start/clp-json.md) flavor of CLP supports queries through
Presto.
:::

:::{note}
This integration with Presto is under development and may change in the future. It is also being
maintained in a [fork][yscope-presto] of the Presto project. At some point, these changes will have
been merged into the main Presto repository so that you can use official Presto releases with CLP.
:::

## Requirements

* [CLP][clp-releases] (clp-json) v0.5.0 or higher
* [Docker] v28 or higher
* [Docker Compose][docker-compose] v2.20.2 or higher
* Python
* python3-venv (for the version of Python installed)

## Set up

Using Presto with CLP requires:

* [Setting up CLP](#setting-up-clp) and compressing some logs.
* [Setting up Presto](#setting-up-presto) to query CLP's metadata database and archives.

### Setting up CLP

1. Follow the [quick-start](quick-start/index.md) guide to download and extract the CLP package,
   but don't start the package just yet.
2. Before starting the package, update the package's config file (`etc/clp-config.yaml`) as follows:

    * Set the `package.query_engine` key to `"presto"`.

      ```yaml
      package:
        storage_engine: "clp-s"
        query_engine: "presto"
      ```

    * Set the `results_cache.retention_period` key to `null` since the CLP + Presto integration
      doesn't yet support garbage collection.

      ```yaml
      results_cache:
      #  host: "localhost"
      #  port: 27017
      #  db_name: "clp-query-results"
      #  stream_collection_name: "stream-files"
      #
      #  # Retention period for search results, in minutes. Set to null to disable automatic deletion.
        retention_period: null
      ```

    * Update the `presto` key with the host and port of the Presto cluster. If you follow the
      [Setting up Presto](#setting-up-presto) section, the host is `localhost` and the port is
      `8889`.

      ```yaml
      presto:
        host: "<ip-address>"
        port: <port>
      ```

      :::{note}
      Presto doesn't need to be running before you start CLP.
      :::

3. If you'd like to store your compressed logs on S3, follow the
   [using object storage](guides-using-object-storage/index.md) guide.

   :::{note}
   Currently, the Presto integration only supports the
   [credentials](guides-using-object-storage/aws-s3/clp-config.md#credentials) authentication type.
   :::

4. Continue following the [quick-start](./quick-start/index.md#using-clp) guide to start CLP and
   compress your logs. A sample dataset that works well with Presto is [postgresql].

### Setting up Presto

1. Clone the CLP repository:

    ```bash
    git clone --branch DOCS_VAR_CLP_GIT_REF https://github.com/y-scope/clp.git
    ```

2. Navigate to the `tools/deployment/presto-clp` directory in your terminal.
3. Generate the necessary config for Presto to work with CLP:

    ```bash
    scripts/set-up-config.sh <clp-json-dir>
    ```

    * Replace `<clp-json-dir>` with the location of the clp-json package you set up in the previous
      section.

4. Configure Presto to use CLP's metadata database as follows:

    * Open and edit `coordinator/config-template/split-filter.json`.
    * For each dataset you want to query, add a filter config of the form:

      ```json
      {
        "clp.default.<dataset>": [
          {
            "columnName": "<timestamp-key>",
            "customOptions": {
              "rangeMapping": {
                "lowerBound": "begin_timestamp",
                "upperBound": "end_timestamp"
              }
            },
            "required": false
          }
        ]
      }
      ```

      * Replace `<dataset>` with the name of the dataset you want to query. (If you didn't specify a
        dataset when compressing your logs, they would be compressed into the `default` dataset.)
      * Replace `<timestamp-key>` with the timestamp key you specified when compressing logs for
        this particular dataset.
    * The complete syntax for this file is [here][clp-connector-docs].

5. Start a Presto cluster by running:

    ```bash
    docker compose up --wait
    ```

    * To use more than one Presto worker, you can use the `--scale` option as follows:

      ```bash
      docker compose up --wait --scale presto-worker=<num-workers>
      ```

      * Replace `<num-workers>` with the number of Presto worker nodes you want to run.

### Stopping the Presto cluster

To stop the Presto cluster:

```bash
docker compose stop
```

To clean up the Presto cluster entirely:

```bash
docker compose down
```

## Querying your logs through Presto

You can query your compressed logs in your browser from [CLP's UI](#querying-from-clps-ui), or
from the command line using the [Presto CLI](#querying-from-the-presto-cli).

Each dataset in CLP shows up as a table in Presto. To show all available datasets:

```sql
SHOW TABLES;
```

:::{note}
If you didn't specify a dataset when compressing your logs in CLP, your logs will have been stored
in the `default` dataset.
:::

To show all available columns in the `default` dataset:

```sql
DESCRIBE default;
```

If you wish to show the columns of a different dataset, replace `default` above.

To query the logs in this dataset:

```sql
SELECT * FROM default LIMIT 1;
```

All kv-pairs in each log event can be queried directly using dot-notation. For example, if your logs
contain the field `foo.bar`, you can query it using:

```sql
SELECT foo.bar FROM default LIMIT 1;
```

### Querying from CLP's UI

CLP's UI should be available at [http://localhost:4000](http://localhost:4000) (if you changed
`webui.host` or `webui.port` in `etc/clp-config.yaml`, use the new values).

:::{note}
The UI can only run one query at a time, and queries must not end with a `;`.
:::

### Querying from the Presto CLI

To access the Presto CLI, navigate to the `tools/deployment/presto-clp` directory and run:

```bash
docker compose exec presto-coordinator \
  presto-cli \
    --catalog clp \
    --schema default
```

## Limitations

The Presto CLP integration has the following limitations at present:

* Nested fields containing special characters cannot be queried (see [y-scope/presto#8]). Allowed
  characters are alphanumeric characters and underscores. To get around this limitation, you'll
  need to preprocess your logs to remove any special characters.

These limitations will be addressed in a future release of the Presto integration.

[clp-connector-docs]: https://docs.yscope.com/presto/connector/clp.html#split-filter-config-file
[clp-releases]: https://github.com/y-scope/clp/releases
[docker-compose]: https://docs.docker.com/compose/install/
[Docker]: https://docs.docker.com/engine/install/
[postgresql]: https://zenodo.org/records/10516401
[Presto]: https://prestodb.io/
[y-scope/presto#8]: https://github.com/y-scope/presto/issues/8
[yscope-presto]: https://github.com/y-scope/presto
