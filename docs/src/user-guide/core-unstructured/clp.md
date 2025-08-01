# clp

For unstructured (plain text) logs, you can compress, decompress, and search them using the `clp`
and `clg` binaries described below.

## Compression

Usage:

```shell
./clp c [<options>] <archives-dir> <input-path> [<input-path> ...]
```

* `archives-dir` is the directory that archives should be written to.
  * `clp` will create a number of files and directories within, so it's best if this directory is
    empty.
  * You can use the same directory repeatedly and `clp` will add to the compressed logs within.
* `input-path` is any plain-text log file or directory containing such files.
* `options` allow you to specify things like a path to a custom
  [schema](../reference-unstructured-schema-file) file (`--schema-path <file-path>`).
  * For a complete list, run `./clp c --help`

### Examples

**Compress `/mnt/logs/log1.log` and output archives to `/mnt/data/archives1`:**

```shell
./clp c /mnt/data/archives1 /mnt/logs/log1.log
```

**Compress `/mnt/logs/log1.log` using a custom schema specified in `/mnt/conf/schemas.txt`:**

```shell
./clp c --schema-path /mnt/conf/schemas.txt /mnt/data/archives1 /mnt/logs/log1.log
```

## Decompression

Usage:

```shell
./clp x [<options>] <archives-dir> <output-dir> [<file-path>]
```

* `archives-dir` is a directory containing archives.
* `output-dir` is the directory that decompressed logs should be written to.
* `file-path` is an optional file path to decompress, in particular.

### Examples

**Decompress all logs from `/mnt/data/archives1` into `/mnt/data/archives1-decomp`:**

```shell
./clp x /mnt/data/archives1 /mnt/data/archives1-decomp
```

**Decompress just `/mnt/logs/file1.log`:**

```shell
./clp x /mnt/data/archives1 /mnt/data/archives1-decomp /mnt/logs/file1.log
```

## Search

Usage:

:::{note}
Search uses a different executable (`clg`) than compression (`clp`).
:::

```shell
./clg [<options>] <archives-dir> <wildcard-query> [<file-path>]
```

* `archives-dir` is a directory containing archives.
* `wildcard-query` is a wildcard query where:
  * the `*` wildcard matches 0 or more characters;
  * the `?` wildcard matches any single character.
* `options` allow you to specify things like a time-range filter.
  * For a complete list, run `./clg --help`

### Examples

**Search `/mnt/data/archives1` for specific ERROR logs and ignore case distinctions:**

```shell
./clg --ignore-case /mnt/data/archives1 " ERROR * container "
```

**Search for logs in a time range:**

```shell
./clg /mnt/data/archives1 --tge 1546344654321 --tle 1546344912345 " user1 "
```

:::{note}
Currently, timestamps must be specified as milliseconds since the UNIX epoch.
:::

**Search a single file**:

```shell
./clg /mnt/data/archives1 " session closed " /mnt/logs/file1
```

# Parallel Compression

:::{warning}
By default, `clp` uses an embedded SQLite database. Running multiple clp instances with SQLite
can fail due to "database is locked" because SQLite does not support concurrent writes.
:::

To enable parallel compression to the same archives directory, `clp`/`clg` can be configured to use
a MySQL-type database (e.g., MariaDB) as follows:

* Install and configure MariaDB using the instructions for your platform
* Create a user who has privileges to create databases, create tables, insert records, and delete
  records.
* Install the MariaDB Python package `pip3 install mariadb`
  * This is necessary to run the database initialization script. If you prefer, you can run the SQL
    statements in `tools/scripts/db/init-db.py` directly.
* Run `tools/scripts/db/init-db.py`. This will initialize the database CLP requires. Use
  command-line options to configure MySQL connection parameters and set environment variables
  for database credentials:
  * `--db-host <host>` to specify the database host (default: `"localhost"`)
  * `--db-port <port>` to specify the database port (default: `3306`)
  * `--db-name <name>` to specify the database name (default: `"clp-db"`)
  * `--db-table-prefix <prefix>` to specify the table prefix (default: `"clp_"`)
  * Set `CLP_DB_USER` environment variable for the database username
  * Set `CLP_DB_PASS` environment variable for the database password
* Run `clp` or `clg` with the same command options and environment variables, with the addition of
  the database type command-line option:
  * `--db-type mysql` to specify MySQL as the database type
* To compress in parallel, simply run another instance of `clp` concurrently.

Note that currently, decompression (`clp x`) and search (`clg`) can only be run with a single
instance. We are in the process of open-sourcing parallelized versions of these as well.
