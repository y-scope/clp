# Using CLP for semi-structured logs

For semi-structured logs (e.g., JSON), you can compress, decompress, and search them using the
`clp-s` binary described below.

## Contents

* [Compression](#compression)
* [Decompression](#decompression)
* [Search](#search)
* [Current limitations](#current-limitations)

## Compression

Usage:

```shell
./clp-s c [<options>] <archives-dir> <input-path> [<input-path> ...]
```

* `archives-dir` is the directory that archives should be written to.
* `input-path` is any new-line-delimited JSON (ndjson) log file or directory containing such files.
* `options` allow you to specify things like which field should be considered as the log event's
  timestamp (`--timestamp-key <field-path>`).
  * For a complete list, run `./clp-s c --help`

### Examples

**Compress `/mnt/logs/log1.json` and output archives to `/mnt/data/archives1`:**

```bash
./clp-s c /mnt/data/archives1 /mnt/logs/log1.json
```

**Treat the field `{"d": {"@timestamp": "..."}}` as each log event's timestamp:**

```bash
./clp-s c --timestamp-key 'd.@timestamp' /mnt/data/archives1 /mnt/logs/log1.json
```

> [!TIP]
> Specifying the timestamp-key will create a range-index for the timestamp column which can increase
> compression ratio and search performance.

**Set the target encoded size to 1 GiB and the compression level to 6 (3 by default)**

```bash
./clp-s c \
    --target-encoded-size 1073741824 \
    --compression-level 6 \
    /mnt/data/archives1 \
    /mnt/logs/log1.json
```

## Decompression

Usage:

```bash
./clp-s x [<options>] <archives-dir> <output-dir>
```

* `archives-dir` is a directory containing archives.
* `output-dir` is the directory that decompressed logs should be written to.
* `options` allow you to specify things like a specific archive (from within `archives-dir`) to
  decompress (`--archive-id <archive-id>`).
  * For a complete list, run `./clp-s x --help`

### Examples

**Decompress all logs from `/mnt/data/archives1` into `/mnt/data/archives1-decomp`:**

```bash
./clp-s x /mnt/data/archives1 /mnt/data/archives1-decomp
```

## Search

Usage:

```bash
./clp-s s [<options>] <archives-dir> <kql-query>
```

* `archives-dir` is a directory containing archives.
* `kql-query` is a [KQL][1] query.
* `options` allow you to specify things like a specific archive (from within `archives-dir`) to
  search (`--archive-id <archive-id>`).
  * For a complete list, run `./clp-s s --help`

### Examples

**Find all log events within a time range:**

```bash
./clp-s s /mnt/data/archives1 'ts >= 1649923037 AND ts <= 1649923038'
```
or
```bash
./clp-s s /mnt/data/archives1 \
    'ts >= date("2022-04-14T07:57:17") AND ts <= date("2022-04-14T07:57:18")'
```

**Find log events with a given key-value pair:**

```bash
./clp-s s /mnt/data/archives1 'id: 22149'
```

**Find ERROR log events containing a substring:**

```bash
./clp-s s /mnt/data/archives1 'level: ERROR AND message: "job*"'
```

**Find FATAL or ERROR log events and ignore case distinctions between values in the query and the
compressed data:**

```bash
./clp-s s --ignore-case /mnt/data/archives1 'level: FATAL OR level: ERROR'
```

## Current limitations

* `clp-s` currently only supports *valid* ndjson logs; it does not handle ndjson logs with trailing
  commas or other JSON syntax errors.
* Time zone information is not preserved.
* The order of log events is not preserved.
* The input directory structure is not preserved and during decompression all files are written to
  the same file.
* The KQL implementation does not fully respect the {} object within array syntax. A matching record
  will satisfy all of the conditions in the filter, but not necessarily on the same object in the
  array. This means that filters like `"a": {"b": 0, "c": 0}` will match documents like
  `{"a": [{"b": 0}, {"c": 0}]}` instead of matching only documents like `{"a": [{"b": 0, "c": 0}]}`
* Searches on arrays must either be fully precise (no wildcard tokens in the key) or fully imprecise
  (a single wildcard token). This means filters like `"*": "uuid"` and `"a.b.c": "uuid"` will search
  inside of array columns, but filters like `"a.*": "uuid"` or `a.*.c: "uuid"` will not.

[1]: https://www.elastic.co/guide/en/kibana/current/kuery-query.html
