This page describes how to use CLP to compress, decompress and search unstructured logs.

# Content

* [Executables](#executables)
    * [`clp`](#clp)
    * [`clg`](#clg)
    * [`make-dictionaries-readable`](#make-dictionaries-readable)
* [Parallel Compression](#parallel-compression)

# Executables

## `clp`

To compress some logs without a schema file:

```shell
./clp c archives-dir /home/my/logs
```

* `archives-dir` is where compressed logs should be output
    * `clp` will create a number of files and directories within, so it's best if this directory is empty
    * You can use the same directory repeatedly and `clp` will add to the compressed logs within.
* `/home/my/logs` is any log file or directory containing log files
* In this mode, `clp` will use heuristics to determine what are the variables in
  each uncompressed message.
    * The heuristics roughly correspond to the example schema file in
      `config/schemas.txt`.

To compress with a user-defined schema file:

```shell
./clp c --schema-path path-to-schema-file archives-dir /home/my/logs 
```

* `path-to-schema-file` is the location of a schema file. For more details on
  schema files, see README-Schema.md.

To decompress those logs:

```shell
./clp x archives-dir decompressed
```

* `archives-dir` is where the compressed logs were previously stored
* `decompressed` is a directory where they will be decompressed to

You can also decompress a specific file:

```shell
./clp x archives-dir decompressed /my/file/path.log
```

* `/my/file/path.log` is the uncompressed file's path (the one that was passed to `clp` for compression)

More usage instructions can be found by running:

```shell
./clp --help
```

## `clg`

To search the compressed logs:

```shell
./clg archives-dir " a *wildcard* search phrase "
```

* `archives-dir` is where the compressed logs were previously stored
* For archives compressed without a schema file:
    * The search phrase can contain the `*` wildcard which matches 0 or more
      characters, or the `?` wildcard which matches any single character.
* For archives compressed using a schema file:
    * `*` may only represent non-delimiter characters.

Similar to `clp`, `clg` can search a single file:

```shell
./clg archives-dir " a *wildcard* search phrase " /my/file/path.log
```

* `/my/file/path.log` is the uncompressed file's path (the one that was passed to `clp` for compression)

More usage instructions can be found by running:

```shell
./clg --help
```

## `make-dictionaries-readable`

If you'd like to convert the dictionaries of an individual archive into a human-readable form, you
can use `make-dictionaries-readable`.

```shell
./make-dictionaries-readable archive-path <output dir>
```

* `archive-path` is a path to a specific archive (inside `archives-dir`)

See the `make-dictionaries-readable` 
[README](../../components/core/src/clp/make_dictionaries_readable/README.md) for details on the 
output format.

# Parallel Compression

By default, `clp` uses an embedded SQLite database, so each directory containing archives can only
be accessed by a single `clp` instance.

To enable parallel compression to the same archives directory, `clp`/`clg` can be configured to
use a MySQL-type database (MariaDB) as follows:

* Install and configure MariaDB using the instructions for your platform
* Create a user that has privileges to create databases, create tables, insert records, and delete
  records.
* Copy and change `config/metadata-db.yml`, setting the type to `mysql` and uncommenting the MySQL
  parameters.
* Install the MariaDB and PyYAML Python packages `pip3 install mariadb PyYAML`
    * This is necessary to run the database initialization script. If you prefer, you can run the
      SQL statements in `tools/scripts/db/init-db.py` directly.
* Run `tools/scripts/db/init-db.py` with the updated config file. This will initialize the
  database CLP requires.
* Run `clp` or `clg` as before, with the addition of the `--db-config-file` option pointing at
  the updated config file.
* To compress in parallel, simply run another instance of `clp` concurrently.

Note that currently, decompression (`clp x`) and search (`clg`) can only be run with a single
instance. We are in the process of open-sourcing parallelized versions of these as well.
