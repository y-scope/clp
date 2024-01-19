# Using GLT for unstructured logs

For unstructured (plain text) logs, you can compress, decompress, and search them using the `glt`
and `gltg` binaries described below.

## Contents

* [Compression](#compression)
* [Decompression](#decompression)
* [Search](#search)
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
    * For a complete list, run `./gltc c --help`

### Examples

**Compress `/mnt/logs/log1.log` and output archives to `/mnt/data/archives1`:**

```shell
./glt c /mnt/data/archives1 /mnt/logs/log1.log
```

**Compress `/mnt/logs/log1.log` using a custom threshold:**

```shell
./clp c --combined-threshold 1 /mnt/data/archives1 /mnt/logs/log1.log
```

> [!TIP]
> The combine-threshold has higher impact on logs with a large number of logtypes.
> In general, a higher combined-threshold results in better compression ratio but lower search speed

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
./clp-s x /mnt/data/archives1 /mnt/data/archives1-decomp
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

## Current limitations

* `clp-s` currently only supports *valid* ndjson logs; it does not handle ndjson logs with trailing
  commas or other JSON syntax errors.
* Time zone information is not preserved.
* The order of log events is not preserved.
* The input directory structure is not preserved and during decompression all files are written to
  the same file.

[1]: https://www.elastic.co/guide/en/kibana/current/kuery-query.html
