This page describes how to use CLP to compress, decompress and search semi-structured logs
(i.e. JSON logs).

# Content
* [Compression](#compression)
* [Decompression](#decompression)
* [Search](#search)
* [Limitations](#limitations)

# Compression

Usage:

```bash
./clp-s c [<options>] <archives-dir> <input-path> [<input-path> ...]
```

+ `archives-dir` is the directory that archives should be written to.
+ `input-path` is any new-line-delimited JSON (ndjson) log file or directory
  containing such files.
+ `options` allow you to specify things like which field should be considered
  the log event's timestamp (`--timestamp-key <field-path>`)
    + For a complete list, run `./clp-s c --help`

## Examples

**Compress `/mnt/logs/log1.json` and outputs archives to `/mnt/data/archives1`:**

```bash
./clp-s c /mnt/data/archives1 /mnt/logs/log1.json
```

**Treat the field `{"d": {"@timestamp": "..."}}` as each log event's
timestamp:**

```bash
./clp-s c --timestamp-key 'd.@timestamp' /mnt/data/archives1 /mnt/logs/log1.json
```

+ Specifying the timestamp-key will create a range-index for the timestamp
  column and can increase compression ratio.

**Set the target encoded size to 1GB and the compression level to 6 (3 by default)**

```bash
./clp-s c --target-encoded-size 1073741824 --compression-level 6 /mnt/data/archives1 /mnt/logs/log1.json
```

# Decompression

Usage:

```bash
./clp-s x [<options>] <archives-dir> <output-dir>
```

+ `archives-dir` is a directory containing archives.
+ `output-dir` is the directory that decompressed logs should be written to.

## Examples

**Decompress archives:**

```bash
./clp-s x /mnt/data/archives1 /mnt/data/archives1-decomp
```

# Search

Usage:

```bash
./clp-s s <archives-dir> <query>
```

+ `archives-dir` is a directory containing archives.
+ `query` is a [KQL][1] query

## Examples

**Find all log events within a time range:**

```bash
./clp-s_search /mnt/data/archives1 'ts >= 1649923037 AND ts <= 1649923038'
```
or
```bash
./clp-s_search /mnt/data/archives1 'ts >= date("2022-04-14T07:57:17") AND ts <= date("2022-04-14T07:57:18")
```

**Find log events with a given key-value pair:**

```bash
./clp-s_search /mnt/data/archives1 'id: 22149'
```

**Find ERROR log events containing a substring:**

```bash
./clp-s_search /mnt/data/archives1 'level: ERROR AND message: "job*"'
```

**Find both FATAL and ERROR log events:**

```bash
./clp-s_search /mnt/data/archives1 'level: FATAL OR level: ERROR'
```

[1]: https://www.elastic.co/guide/en/kibana/current/kuery-query.html

# Limitations
* `clp-s` currently only supports ndjson logs in correct JSON format. It does not handle
  ndjson logs with trailing commas or other JSON syntax errors.
* Timezone information is not preserved.
* The order of log events is not preserved.
* The input directory structure is not preserved and during decompression all
  files are written to the same file.