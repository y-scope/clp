# CLP for JSON logs

For JSON logs, you can compress, decompress, and search them using the `clp-s` binary described
below.

## Compression

Usage:

```shell
./clp-s c [<options>] <archives-dir> <input-path> [<input-path> ...]
```

* `archives-dir` is the directory that archives should be written to.
* `input-path` is a filesystem path or URL to either:
  * a new-line-delimited JSON (ndjson) log file;
  * a KV-IR file; or
  * a directory containing such files.
* `options` allow you to specify how data gets compressed into an archive. For example:
  * `--single-file-archive` specifies that single-file archives should be produced (i.e., each
    archive is a single file in `archives-dir`).
  * `--file-type <json|kv-ir>` specifies whether the input files are encoded as ndjson or KV-IR.
  * `--timestamp-key <field-path>` specifies which field should be treated as each log event's
    timestamp.
  * `--target-encoded-size <size>` specifies the threshold (in bytes) at which archives are split,
    where `size` is the total size of the dictionaries and encoded messages in an archive.
    * This option acts as a soft limit on memory usage for compression, decompression, and search.
    * This option significantly affects compression ratio.
  * `--retain-float-format` specifies that float numbers should be stored with format information
    to allow retaining original float numbers' formats after decompression (see “Current
    limitations” below and the design notes in the dev guide). Note: This currently applies to
    JSON files; KV‑IR support for preserving original printed float formats is under evaluation.
  * `--structurize-arrays` specifies that arrays should be fully parsed and array entries should be
    encoded into dedicated columns.
  * `--auth <s3|none>` specifies the authentication method that should be used for network requests
    if the input path is a URL.
    * When S3 authentication is enabled, we issue a GET request following the [AWS Signature Version
      4 specification][aws-signature-v4]. This request uses the environment variables
      `AWS_ACCESS_KEY_ID`, `AWS_SECRET_ACCESS_KEY`, and, optionally, `AWS_SESSION_TOKEN` if it
      exists.
    * For more information on usage with S3, see our
      [dedicated guide](guides-using-object-storage/index).

For a complete list of options, run `./clp-s c --help`.

### Examples

**Compress `/mnt/logs/log1.json` and output archives to `/mnt/data/archives1`:**

```shell
./clp-s c /mnt/data/archives1 /mnt/logs/log1.json
```

**Treat the field `{"d": {"@timestamp": "..."}}` as each log event's timestamp:**

```shell
./clp-s c --timestamp-key 'd.@timestamp' /mnt/data/archives1 /mnt/logs/log1.json
```

:::{tip}
Specifying the timestamp-key will create a range-index for the timestamp column which can increase
compression ratio and search performance.
:::

**Compress a KV-IR file stored on S3 into a single-file archive:**

```shell
AWS_ACCESS_KEY_ID='...' AWS_SECRET_ACCESS_KEY='...' \
  ./clp-s c --single-file-archive --file-type kv-ir --auth s3 /mnt/data/archives \
  https://my-bucket.s3.us-east-2.amazonaws.com/kv-ir-log.clp
```

**Set the target encoded size to 1 GiB and the compression level to 6 (3 by default)**

```shell
./clp-s c \
    --target-encoded-size 1073741824 \
    --compression-level 6 \
    /mnt/data/archives1 \
    /mnt/logs/log1.json
```

:::{tip}
Use the `--retain-float-format` flag during compression. When present in an archive, decompression
automatically uses the stored format metadata so outputs match the original text (subject to the
limitations below). For example, values like `1.000e+00` or `0.000000012300` will be printed
unchanged.
:::

**Enable retaining float numbers' formats:**

```shell
./clp-s c \
    --retain-float-format \
    /mnt/data/archives1 \
    /mnt/logs/log1.json
```

## Decompression

Usage:

```shell
./clp-s x [<options>] <archives-path> <output-dir>
```

* `archives-path` is a directory containing archives, a path to an archive, or a URL pointing to a
  single-file archive.
* `output-dir` is the directory that decompressed logs should be written to.
* `options` allow you to specify things like a specific archive (from within `archives-path`, if it
  is a directory) to decompress (`--archive-id <archive-id>`).
  * For a complete list, run `./clp-s x --help`

### Examples

**Decompress all logs from `/mnt/data/archives1` into `/mnt/data/archives1-decomp`:**

```shell
./clp-s x /mnt/data/archives1 /mnt/data/archives1-decomp
```

## Search

Usage:

```shell
./clp-s s [<options>] <archives-path> <kql-query>
```

* `archives-path` is a directory containing archives, a path to an archive, or a URL pointing to a
  single-file archive.
* `kql-query` is a [KQL](reference-json-search-syntax) query.
* `options` allow you to specify things like a specific archive (from within `archives-path`, if it
  is a directory) to search (`--archive-id <archive-id>`).
  * For a complete list, run `./clp-s s --help`

### Examples

**Find all log events within a time range:**

```shell
./clp-s s /mnt/data/archives1 'ts >= 1649923037 AND ts <= 1649923038'
```
or
```shell
./clp-s s /mnt/data/archives1 \
    'ts >= date("2022-04-14T07:57:17") AND ts <= date("2022-04-14T07:57:18")'
```

**Find log events with a given key-value pair:**

```shell
./clp-s s /mnt/data/archives1 'id: 22149'
```

**Find ERROR log events containing a substring:**

```shell
./clp-s s /mnt/data/archives1 'level: ERROR AND message: "job*"'
```

**Find FATAL or ERROR log events and ignore case distinctions between values in the query and the
compressed data:**

```shell
./clp-s s --ignore-case /mnt/data/archives1 'level: FATAL OR level: ERROR'
```

## Current limitations

* `clp-s` currently only supports *valid* JSON logs; it does not handle JSON logs with trailing
  commas or other JSON syntax errors.
* Time zone information is not preserved.
* The order of log events is not preserved.
* The input directory structure is not preserved and during decompression all files are written to
  the same file.
* When using the `--retain-float-format` flag:
  * For both standard decimal and scientific notation, the maximum number of significant digits is
    **16** (e.g., `123456789.12345678` will be rounded to `123456789.1234567`). Values exceeding
    this limit will be rounded.
  * In scientific notation, the exponent’s printed width can have at most **4 digits**; any extra
    leading zeros will be removed (e.g., `1.0e00000` becomes `1.0e0000`).
  * In scientific notation, the mantissa (significand) must be between **1** and **9** (or **0**
    only if the number equals 0). If it falls outside this range, the output mantissa will differ
    from the input, as only values within 0 to 9 are supported (e.g., `123456789.12345678E3`
    becomes `1.234567891234568E8`, which is also rounded because it has more than 16 significant
    digits).
  * KV-IR inputs currently don't support preserving original printed float formats, only JSON
    file is suppoerted.
* In addition, there are a few limitations, related to querying arrays, described in the search
  syntax [reference](reference-json-search-syntax).

[aws-signature-v4]: https://docs.aws.amazon.com/AmazonS3/latest/API/sigv4-query-string-auth.html
