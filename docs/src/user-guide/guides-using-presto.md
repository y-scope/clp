# Using Presto with CLP

[Presto] is a distributed SQL query engine that can be used to query data stored in CLP (using SQL).
This guide describes how to set up and use Presto with CLP.

:::{warning}
Currently, only the [clp-json](quick-start/clp-json.md) flavor of CLP supports queries through
Presto.
:::

:::{note}
Currently, this integration with Presto is under development and may change in the future. It is
also being maintained in a [fork][yscope-presto] of the Presto project. We are working on merging
these changes into the main Presto repository so that you can use official Presto releases with CLP.
:::

## Requirements

* [CLP][clp-releases] (clp-json) v0.4.0 or higher
* [Docker] v28 or higher
* [Docker Compose][docker-compose] v2.20.2 or higher
* Python
* python3-venv (for the version of Python installed)

## Set up

Using Presto with CLP requires:

* [Setting up CLP](#setting-up-clp) and compressing some logs.
* [Setting up Presto](#setting-up-presto) to query CLP's metadata database and archives.

### Setting up CLP

Follow the [quick-start](./quick-start/index.md) guide to set up CLP and compress your logs. A
sample dataset that works well with Presto is [postgresql].

### Setting up Presto

1. Clone the CLP repository:

    ```bash
    git clone https://github.com/y-scope/clp.git
    ```

2. Navigate to the `tools/deployment/presto-clp` directory in your terminal.
3. Run the following script to generate the necessary config for Presto to work with CLP:

    ```bash
    scripts/set-up-config.sh <clp-json-dir>
    ```

    * `<clp-json-dir>` is the location of the clp-json package you set up in the previous section.

    Note that for the metadata filter config (i.e., 
    `tools/deployment/presto-clp/coordinator/config-template/metadata-filter.json`), it is a config
    to indicate which columns are used for filtering splits that will be processed by Presto. Here
    is an example:

    ```json
    {
      "clp.default.default": [
        {
          "columnName": "timestamp",
          "rangeMapping": {
            "lowerBound": "begin_timestamp",
            "upperBound": "end_timestamp"
          },
          "required": false
        }
      ]
    }
    ```

    * `"clp.default.default"` is the filter's scope. A scope can be one of the following:

      * A catalog name
      * A fully-qualified schema name
      * A fully-qualified table name

      Filter configs under a particular scope will apply to all child scopes. For example, filter
      configs at the schema level will apply to all tables within that schema. In this example,
      the filter will only apply to the `default` table under the `default` schema of the `clp`
      catalog.

    * `"columnName"` is the data column's name. You can use the column used as `--timestamp-key`
    when compressing if you want to filter splits by timestamp.

    * `"rangeMapping"` is an optional object with the following properties:

      * `"lowerBound"` is the metadata column that represents the lower bound of values in a split
      for the data column.
      * `"upperBound"` is the metadata column that represents the upper bound of values in a split
      for the data column.

      In this example, since in CLP's metadata database, for each split (i.e., archive) there are
      two fields `begin_timestamp` and `end_timestamp` to store the earilest and latest timestamps
      of the log messages compressed in that split, we have to remap the original data column's
      name to these two fields so that it can query the metadata database to retrieve filtered
      splits.

    * `"required"` is an optional field (defaults to false) which indicates whether the filter must
    be present in the translated metadata filter SQL query. If a required filter is missing or
    cannot be pushed down, the query will be rejected.

4. Start a Presto cluster by running:

    ```bash
    docker compose up
    ```

    * To use more than Presto worker, you can use the `--scale` option as follows:

      ```bash
      docker compose up --scale presto-worker=<num-workers>
      ```

      * `<num-workers>` is the number of Presto worker nodes you want to run.

### Stopping the Presto cluster

To stop the Presto cluster, use CTRL + C.

To clean up the Presto cluster entirely:

```bash
docker compose rm
```

## Querying your logs through Presto

To query your logs through Presto, you can use the Presto CLI:

```bash
docker compose exec presto-coordinator \
  presto-cli \
    --catalog clp \
    --schema default
```

Each dataset in CLP shows up as a table in Presto. To show all available datasets:

```sql
SHOW TABLES;
```

If you didn't specify a dataset when compressing your logs in CLP, your logs will have been stored
in the `default` dataset. If you also didn't specify any metadata filters, you can query the logs
in this dataset:

```sql
SELECT * FROM default LIMIT 1;
```

All kv-pairs in each log event can be queried directly using dot-notation. For example, if your logs
contain the field `foo.bar`, you can query it using:

```sql
SELECT foo.bar FROM default LIMIT 1;
```

## Limitations

The Presto CLP integration has the following limitations at present:

* Nested fields containing special characters (i.e., any non-alphanumeric characters except `_`;
see [y-scope/presto#8]). To get around this limitation,you will need to preprocess your logs to
remove such special characters.
* Only logs stored on the filesystem, rather than S3, can be queried through Presto.

These limitations will be addressed in a future release of the Presto integration.

[clp-releases]: https://github.com/y-scope/clp/releases
[docker-compose]: https://docs.docker.com/compose/install/
[Docker]: https://docs.docker.com/engine/install/
[postgresql]: https://zenodo.org/records/10516401
[Presto]: https://prestodb.io/
[y-scope/presto#8]: https://github.com/y-scope/presto/issues/8
[yscope-presto]: https://github.com/y-scope/presto
