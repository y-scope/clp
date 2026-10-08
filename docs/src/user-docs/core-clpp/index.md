# CLP+ (experimental)

CLP+ is an experimental extension of `clp-s` that parses unstructured text logs into structured,
queryable fields. Rather than storing each log message as an opaque string, CLP+ splits it into a
reusable template (log shape) and the leaf rule matches that fill it in, improving compression and
letting you query the individual parts of a message.

:::{warning}
CLP+ is experimental. The CLI flags, output format, and behaviour described here may change without
notice.
:::

## Terminology

CLP+ parses each log message with [log-surgeon][log-surgeon], using rules supplied in a
[parsing specification](parsing-spec). These terms are used throughout the CLP+ documentation:

* A **log message** is an unstructured text field of a JSON log event.
* A **parsing specification** is the set of rules log-surgeon uses to build a parser.
* A **rule** is a name paired with a regular expression.
  * A **root rule** is a top-level entry in the parsing specification.
  * A **sub-rule** is a named capture group within another rule.
  * A **parent rule** is a rule that contains sub-rules.
  * A **leaf rule** contains no sub-rules.
* A **match** is one occurrence of a rule in a log message.
* A **shape** is a sequence of static text and leaf rule placeholders, written as `%full.name%`.
  * The **log shape** is the shape of a whole log message.
  * A **rule shape** is the shape of a parent rule match.

For example, given the log message `"block: blk_1073746484_5660"` in a field named `"message"` and a
root rule `block_id: "blk_(?<num>\d+)_(?<gen_stamp>\d+)"`, CLP+ produces:

```text
log shape: "block: blk_%block_id.num%_%block_id.gen_stamp%"
block_id parent rule shape: "blk_%block_id.num%_%block_id.gen_stamp%"
block_id.num: 1073746484
block_id.gen_stamp: 5660
```

A leaf rule's fully qualified name becomes its **column** path, prefixed by the field the text came
from. The two matches above are therefore queryable as `message.block_id.num` and
`message.block_id.gen_stamp`. All rules may match more than once per log message, which is why they
appear as arrays in [search output](search.md#output-format).

## Enabling CLP+

Every CLP+ command requires `--experimental`. Compression additionally requires
`--parsing-specification`, but extraction and search do not as the spec is stored in the archive.

:::{note}
Reading a CLP+ archive without `--experimental` fails with an error. Passing `--experimental` when
reading a regular `clp-s` archive is harmless.
:::

## Compression

Usage:

```shell
./clp-s c --experimental --parsing-specification=<spec-path> [<options>] \
    <archives-dir> <input-path> [<input-path> ...]
```

* `--experimental` enables CLP+.
* `--parsing-specification <spec-path>` is the [parsing specification](parsing-spec) used to parse
  log messages.
* All standard `clp-s c` options still apply. For a complete list, run `./clp-s c --help`.

:::{note}
`--experimental` and `--parsing-specification` must be used together. Supplying only one is an
error.
:::

### Examples

**Compress `/mnt/logs/app.log` into `/mnt/data/archives`:**

```shell
./clp-s c --experimental \
    --parsing-specification=./parsing-spec.txt \
    /mnt/data/archives \
    /mnt/logs/app.log
```

**Compress JSON logs where the timestamp lives in its own field:**

```shell
./clp-s c --experimental \
    --parsing-specification=./parsing-spec.txt \
    --timestamp-key timestamp \
    /mnt/data/archives \
    /mnt/logs/app.jsonl
```

## Extraction

Usage:

```shell
./clp-s x --experimental [<options>] <archives-path> <output-dir>
```

Extraction reconstructs each log message from its log shape and leaf rule matches.

* `--ordered` preserves the original order of log events.
* All standard `clp-s x` options still apply. For a complete list, run `./clp-s x --help`.

### Examples

**Extract all logs from a CLP+ archive directory, preserving order:**

```shell
./clp-s x --experimental --ordered /mnt/data/archives /mnt/data/extracted
```

**Extract a single archive by ID:**

```shell
./clp-s x --experimental --archive-id <archive-id> /mnt/data/archives /mnt/data/extracted
```

## Searching

See [Searching CLP+ archives](search.md).

## Limitations

* CLP+ inherits all limitations from CLP-S, such as:
  * Keys within a JSON object may be reordered by a compress-and-extract round trip. Values are
    unchanged.
  * Without `--ordered`, log events are not extracted in their original order.
  * The input directory structure is not preserved; extraction writes all messages to a single
    output file.
* Some search features require an opt-in build. See
  [Filters that require an opt-in build](search.md#filters-that-require-an-opt-in-build).

:::{toctree}
:hidden:

search
parsing-spec
:::

[log-surgeon]: https://github.com/y-scope/log-surgeon
