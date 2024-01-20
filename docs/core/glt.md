# Using GLT for unstructured logs

For unstructured (plain text) logs, you can compress, decompress, and search them using the `glt`
and `gltg` binaries described below.

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
* `input-path` is any new-line-delimited JSON (ndjson) log file or directory containing such files.
* `options` allow you to specify things like a custom percentage threshold for combined logtype tables
  (`--combine-threshold <threshold>`).
    * For a complete list, run `./glt c --help`

### Examples

**Compress `/mnt/logs/log1.log` and output archives to `/mnt/data/archives1`:**

```shell
./glt c /mnt/data/archives1 /mnt/logs/log1.log
```

**Compress `/mnt/logs/log1.log` using a custom threshold of 1%:**

```shell
./glt c --combined-threshold 1 /mnt/data/archives1 /mnt/logs/log1.log
```

> [!TIP]
> The combine-threshold has a more obvious effect on logs with a large number of logtypes.
> In general, a higher combined-threshold results in better compression ratio and lower search speed.

## Decompression

Usage:

```bash
./glt x <archives-dir> <output-dir>
```

* `archives-dir` is a directory containing archives.
* `output-dir` is the directory that decompressed logs should be written to.

### Examples

**Decompress all logs from `/mnt/data/archives1` into `/mnt/data/archives1-decomp`:**

```bash
./glt x /mnt/data/archives1 /mnt/data/archives1-decomp
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
> Adding spaces (when possible) at the begin and the end of the wildcard-query can improve GLT's search performance,
> as GLT doesn't need to consider implicit wildcards during query processing.
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

* Timestamp information is not preserved in search results. All search results use a default timestamp format.
* The order of log events is not preserved in search results.