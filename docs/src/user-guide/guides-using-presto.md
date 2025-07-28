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
sample dataset that works well with Presto is the [postgresql] dataset.

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

If you want to clean up the Presto cluster entirely:

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
in the `default` dataset. To query the logs in this dataset:

```sql
SELECT * FROM default LIMIT 1;
```

All kv-pairs in each log event can be queried directly using dot-notation. For example, if your logs
contain the field `foo.bar`, you can query it using:

```sql
SELECT foo.bar FROM default LIMIT 1;
```

## Limitations

The Presto CLP integration the following limitations at present:

* Nested-fields that contain special characters (e.g., `t.$date`, where `$` is considered a special
  character by Presto) cannot be queried (see [y-scope/presto#8]). To get around this limitation,
  you will need to preprocess your logs to remove such special characters.
* Only logs stored on the filesystem, rather than S3, can be queried through Presto.

These limitations will be addressed in a future release of the Presto integration.

[docker-compose]: https://docs.docker.com/compose/install/
[Docker]: https://docs.docker.com/engine/install/
[postgresql]: https://zenodo.org/records/10516401
[Presto]: https://prestodb.io/
[y-scope/presto#8]: https://github.com/y-scope/presto/issues/8
[yscope-presto]: https://github.com/y-scope/presto
