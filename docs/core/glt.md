# Using GLT for unstructured logs

GLT (Group-by Log Type) is a version of CLP specialized for enhanced search performance, at the 
cost of higher memory usage during compression. During compression, log events with the same log 
type are grouped together into tables that can be compressed and searched more efficiently. In our
benchmarks, compared to CLP, we found that GLT's compression ratio is 1.24x higher and searches 
are 7.8x faster on average.

You can use GLT to compress, decompress, and search unstructured (plain-text) logs using the `glt`
binary described below.

## Contents

* [Compression](#compression)
* [Decompression](#decompression)
* [Search](#search)
* [Utilities](#utilities)
  * [`make-dictionaries-readable`](#make-dictionaries-readable)
* [Current limitations](#current-limitations)

## Compression

Usage:

```shell
./glt c [<options>] <archives-dir> <input-path> [<input-path> ...]
```

* `archives-dir` is the directory that archives should be written to.
  * `glt` will create a number of files and directories within, so it's best if this directory is
    empty.
  * You can use the same directory repeatedly and `glt` will add to the compressed logs within.
* `input-path` is any plain-text log file or directory containing such files.
* `options` allow you to specify things like the level of compression to apply.
    * For a complete list, run `./glt c --help`

### Examples

**Compress `/mnt/logs/log1.log` and output archives to `/mnt/data/archives1`:**

```shell
./glt c /mnt/data/archives1 /mnt/logs/log1.log
```

## Decompression

Usage:

```shell
./glt x [<options>] <archives-dir> <output-dir> [<file-path>]
```

* `archives-dir` is a directory containing archives.
* `output-dir` is the directory that decompressed logs should be written to.
* `file-path` is an optional file path to decompress, in particular.

### Examples

**Decompress all logs from `/mnt/data/archives1` into `/mnt/data/archives1-decomp`:**

```shell
./glt x /mnt/data/archives1 /mnt/data/archives1-decomp
```

**Decompress just `/mnt/logs/file1.log`:**

```shell
./glt x /mnt/data/archives1 /mnt/data/archives1-decomp /mnt/logs/file1.log
```

## Search

Usage:

```shell
./glt s [<options>] <archives-dir> <wildcard-query> [<file-path>]
```

* `archives-dir` is a directory containing archives.
* `wildcard-query` is a wildcard query where:
    * the `*` wildcard matches 0 or more characters;
    * the `?` wildcard matches any single character.
* `options` allow you to specify things like a time-range filter.
    * For a complete list, run `./glt s --help`

> [!TIP]
> Adding spaces (when possible) at the beginning and the end of the wildcard-query can improve GLT's 
> search performance, since GLT won't need to consider implicit wildcards during query processing.
> For example, the query " ERROR * container " is preferred to "ERROR * container".

### Examples

**Search `/mnt/data/archives1` for specific ERROR logs:**

```shell
./glt s /mnt/data/archives1 " ERROR * container "
```

**Search for logs in a time range:**

```shell
./glt s /mnt/data/archives1 --tge 1546344654321 --tle 1546344912345 " user1 "
```

> [!NOTE]
> Currently, timestamps must be specified as milliseconds since the UNIX epoch.

**Search a single file**:

```shell
./clg /mnt/data/archives1 " session closed " /mnt/logs/file1
```

# Utilities

Below are utilities for working with GLT archives.

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


## Current limitations

* Timestamp format information is not preserved in search results. Instead, all search results use a
  default timestamp format.
* Search results are not output in the same order that they were in the original log files.
