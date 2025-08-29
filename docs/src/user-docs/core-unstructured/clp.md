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

To enable parallel compression to the same archives directory, `clp` (and by extension, `clg`) needs
to be configured to use a MySQL-compatible database (e.g., MariaDB) rather than the default---an
embedded SQLite database (which doesn't support concurrent writes).

:::{warning}
Running multiple `clp` instances with SQLite can fail due to the error "database is locked".
:::

You can configure `clp` and `clg` to use a MySQL-compatible database as follows:

* Install and configure MariaDB using the instructions for your platform
* Create a user that has privileges to create databases, create tables, insert records, and delete
  records.
* Install the MariaDB Python package: `pip3 install mariadb`
  * This is necessary to run the database initialization script. If you prefer, you can run the SQL
    statements in `tools/scripts/db/init-db.py` directly.
* Run `tools/scripts/db/init-db.py` to initialize the database that CLP requires. Use the following
  command-line options to configure database connection parameters and set environment variables for
  database credentials. If a command-line option isn't specified, its default value will be used:
  * `--db-host <host>` to specify the database host
  * `--db-port <port>` to specify the database port
  * `--db-name <name>` to specify the database name
  * `--db-table-prefix <prefix>` to specify the table prefix
  * Set the `CLP_DB_USER` environment variable for the database user's username
  * Set the `CLP_DB_PASS` environment variable for the database user's password
* Run `clp` or `clg` with the same command-line options and environment variables, with the addition
  of the database type command-line option:
  * `--db-type mysql` to specify MySQL as the database type

To compress logs in parallel, run as many parallel instances of `clp` as desired.

Note that currently, decompression (`clp x`) and search (`clg`) can only be run with a single
instance. We are in the process of open-sourcing parallelized versions of these as well.
