# Using CLP for unstructured logs

For unstructured (plain text) logs, you can compress, decompress, and search them using the `clp`
and `clg` binaries described below.

## Contents

* [Compression](#compression)
* [Decompression](#decompression)
* [Search](#search)
* [Parallel compression](#parallel-compression)
* [Utilities](#utilities)
  * [`make-dictionaries-readable`](#make-dictionaries-readable)

## Compression

### `clp`

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
  [schema](../../components/core/README-Schema.md) file (`--schema-path <file-path>`).
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

> [!NOTE]
> Search uses a different executable (`clg`) than compression (`clp`).

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

**Search `/mnt/data/archives1` for specific ERROR logs:**

```shell
./clg /mnt/data/archives1 " ERROR * container "
```

**Search for logs in a time range:**

```shell
./clg /mnt/data/archives1 --tge 1546344654321 --tle 1546344912345 " user1 "
```

> [!NOTE]
> Currently, timestamps must be specified as milliseconds since the UNIX epoch.

**Search a single file**:

```shell
./clg /mnt/data/archives1 " session closed " /mnt/logs/file1
```

# Parallel Compression

By default, `clp` uses an embedded SQLite database, so each directory containing archives can only
be accessed by a single `clp` instance.

To enable parallel compression to the same archives directory, `clp`/`clg` can be configured to use
a MySQL-type database (e.g., MariaDB) as follows:

* Install and configure MariaDB using the instructions for your platform
* Create a user that has privileges to create databases, create tables, insert records, and delete
  records.
* Copy and change `config/metadata-db.yml`, setting the type to `mysql` and uncommenting the MySQL
  parameters.
* Install the MariaDB and PyYAML Python packages `pip3 install mariadb PyYAML`
  * This is necessary to run the database initialization script. If you prefer, you can run the SQL
    statements in `tools/scripts/db/init-db.py` directly.
* Run `tools/scripts/db/init-db.py` with the updated config file. This will initialize the database
  CLP requires.
* Run `clp` or `clg` as before, with the addition of the `--db-config-file` option pointing at the
  updated config file.
* To compress in parallel, simply run another instance of `clp` concurrently.

Note that currently, decompression (`clp x`) and search (`clg`) can only be run with a single
instance. We are in the process of open-sourcing parallelized versions of these as well.

# Utilities

Below are utilities for working with CLP archives. 

## `make-dictionaries-readable`

To convert the dictionaries of an individual archive into a human-readable form, you can use
`make-dictionaries-readable`.

```shell
./make-dictionaries-readable archive-path <output dir>
```

* `archive-path` is a path to a specific archive (inside `archives-dir`)

See the `make-dictionaries-readable` 
[README](../../components/core/src/clp/make_dictionaries_readable/README.md) for details on the 
output format.
