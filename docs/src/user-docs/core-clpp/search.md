# Searching CLP+ archives

CLP+ archives are searched with [KQL](../reference-json-search-syntax), the same query language used
by CLP-S. Because each log message is parsed into named rules (see
[Terminology](index.md#terminology)), you can query those rule matches directly rather than
substring-matching the whole log message.

Usage:

```shell
./clp-s s --experimental [<options>] <archives-path> <kql-query>
```

* `--projection <column> ...` selects which parts of a result to output. See
  [Projections](#projections).
* All standard `clp-s s` options still apply. For a complete list, run `./clp-s s --help`.

## Querying leaf rule matches

Each leaf rule is a column addressed by its dotted path. Given a log shape of
`blk_%block_id.num%_%block_id.gen_stamp%` on the `message` field:

```shell
# Exact match on a leaf rule
./clp-s s --experimental /mnt/data/archives 'message.block_id.gen_stamp: 5660'

# Wildcards
./clp-s s --experimental /mnt/data/archives 'message.block_id.gen_stamp: 566*'

# Numeric comparison; matches that look like numbers are stored as numbers
./clp-s s --experimental /mnt/data/archives 'message.block_id.num > 1073746400'

# Existence
./clp-s s --experimental /mnt/data/archives 'message.block_id.num: *'

# Combine with AND/OR, like any KQL query
./clp-s s --experimental /mnt/data/archives \
    'message.block_id.num: 1073746400 and message.block_id.gen_stamp: 566*'
```

## Matching rule shapes with `shape()`

`shape(column)` matches against the shape rather than the leaf rule matches, which is useful for
finding all log messages of a given form regardless of the matches they carry.

```shell
# Log messages whose log shape contains this static text
./clp-s s --experimental /mnt/data/archives 'shape(message): "*to MEMORY*"'

# Log messages containing a block_id whose rule shape starts with blk_
./clp-s s --experimental /mnt/data/archives 'shape(message.block_id): "blk_*"'
```

`column` must be a log message or a parent rule. Applying `shape()` to a leaf rule is an error:

```text
shape(<col>) can only be applied to LogMessage or ParentRule columns; no LogMessage or
ParentRule nodes match column "message.block_id.gen_stamp".
```

`shape()` may be used both as a filter and as a [projection](#projections).

## Wildcards in column names

A `*` in place of a column name matches every column at that position, and the query is the union of
the results from each. Every matched column behaves exactly as if you had written its full dotted
path. Therefore, a wildcard that reaches a log message or a parent rule searches that log message
or parent rule, any child parent rules, and any child leaf rules.

:::{note}
Because a wildcard can reach a log message or a parent rule, `message.*` and `message.block_id.*`
take on the text queries described in
[Filters that require an opt-in build](#filters-that-require-an-opt-in-build). Wildcards that
resolve only to leaf rules, such as `message.*.gen_stamp`, are unaffected.
:::

## Filters that require an opt-in build

Filtering on the *text* of a log message or a parent rule match requires **query decomposition**:
working out which combinations of leaf rule matches could produce the queried text.

:::{warning}
Query decomposition is still under development and is **not yet reliable**. It is disabled by
default, and running one of these queries against a default build aborts the search with an error.
To enable it, build with `-DCLP_BUILD_CLPP_DECOMPOSITION=ON`.
:::

| Query                                                          | Default build | Needs decomposition |
| -------------------------------------------------------------- | :-----------: | :-----------------: |
| `message.block_id.gen_stamp: 566*` (leaf rule match)           |      yes      |          no         |
| `message.*.gen_stamp: 566*` (leaf rule wildcard)               |      yes      |          no         |
| `shape(message): "*to MEMORY*"` (log shape)                    |      yes      |          no         |
| `shape(message.block_id): "blk_*"` (rule shape)                |      yes      |          no         |
| `message: *` (existence)                                       |      yes      |          no         |
| `message: "*blk_1073746491_5667*"` (log message text)          |       no      |         yes         |
| `message.block_id: "*blk_*"` (parent rule match text)          |       no      |         yes         |
| `message.block_id.*: *blk*` (wildcard reaching a parent rule)  |       no      |         yes         |
| `message.*: "566"` (wildcard reaching the log message)         |       no      |         yes         |

Everything else (i.e. projection, compression, and extraction) is unaffected by this flag.

:::{note}
`--ignore-case` is ignored when filtering on log message text. Those queries are always
case-sensitive. It works as expected for `shape()` filters and parent rule match text queries.
:::

## Projections

`--projection` selects which parts of a result to output, and must come after all positional
arguments. Alongside plain column names, two functions are available:

* `shape(column)` outputs the shape.
* `decompose(column)` outputs the shape plus every leaf rule match beneath the column.
  Note, `decompose(X)` implies `shape(X)`.

Both require a log message or parent rule column, and both produce the error shown
[above](#matching-rule-shapes-with-shape) when applied to a leaf rule. `decompose()` is
projection-only; it cannot be used as a filter.

### Output format

A CLP+ log message is always emitted as a JSON object, never a bare string. Which sub-keys appear
depends on what you project:

* `text` is the reconstructed original text.
* `shape` is the log shape, or the rule shape when projected at a parent rule.
* Leaf rule matches are one array per leaf rule, since a leaf rule may match several times per log
  message.
* Parent rules are one array per parent rule, holding one object per parent rule match.

The examples below use a log message with the log shape
`blk_%block_id.num%_%block_id.gen_stamp%` and the leaf rule matches `1073746484` and `5660`.

| `--projection`                                 | Output                                                                                                   |
| ---------------------------------------------- | -------------------------------------------------------------------------------------------------------- |
| *(omitted)*                                    | `{"message": {"text": "blk_1073746484_5660"}}`                                                           |
| `message`                                      | `{"message": {"text": "blk_1073746484_5660"}}`                                                           |
| `shape(message)`                               | `{"message": {"shape": "blk_%block_id.num%_%block_id.gen_stamp%"}}`                                      |
| `decompose(message)`                           | `{"message": {"shape": "...", "block_id": [{"num": [1073746484], "gen_stamp": [5660]}]}}`                |
| `message shape(message)`                       | `{"message": {"text": "...", "shape": "..."}}`                                                           |
| `message decompose(message)`                   | `{"message": {"text": "...", "shape": "...", "block_id": [...]}}`                                        |
| `message.block_id`                             | `{"message": {"block_id": [{"text": "blk_1073746484_5660"}]}}`                                           |
| `message.block_id.gen_stamp`                   | `{"message": {"block_id": [{"gen_stamp": [5660]}]}}`                                                     |
| `message.block_id message.block_id.gen_stamp`  | `{"message": {"block_id": [{"text": "...", "gen_stamp": [5660]}]}}`                                      |
| `shape(message.block_id)`                      | `{"message": {"block_id": [{"shape": "blk_%block_id.num%_%block_id.gen_stamp%"}]}}`                      |
| `decompose(message.block_id)`                  | `{"message": {"block_id": [{"shape": "...", "num": [1073746484], "gen_stamp": [5660]}]}}`                |
| `message.block_id decompose(message.block_id)` | `{"message": {"block_id": [{"text": "...", "shape": "...", "num": [...], "gen_stamp": [...]}]}}`         |
| `message decompose(message.block_id)`          | `{"message": {"text": "...", "block_id": [{"shape": "...", "num": [...], "gen_stamp": [...]}]}}`         |

Two noteworthy details:

* **`decompose()` applies only where you put it.** `decompose(message.block_id)` outputs `block_id`
  and everything under it, but does not add `message.text` or `message.shape`, and does not output
  other parent rules in the log message. To get the text as well, project both, as in the last row.
* **`decompose()` reaches all the way down.** Applied to a log message, it emits every leaf rule
  match at every nesting depth, cascading through nested parent rules.

### Examples

```shell
# Shape only
./clp-s s --experimental /mnt/data/archives 'message.block_id: *' \
    --projection "shape(message)"

# Full structural breakdown
./clp-s s --experimental /mnt/data/archives 'message.block_id: *' \
    --projection "decompose(message)"

# Original text alongside the breakdown
./clp-s s --experimental /mnt/data/archives 'message.block_id: *' \
    --projection "message" "decompose(message)"
```

:::{note}
Projection applies to search only. `clp-s x` always extracts the original log message text.
:::

## Archive statistics

Three queries report archive metadata instead of log events. All require `--experimental` but work
on both CLP+ and regular `clp-s` archives. Each output line carries an `archive_id` so results from
multiple archives can be told apart.

```shell
./clp-s s --experimental /mnt/data/archives 'stats.archives'
./clp-s s --experimental /mnt/data/archives 'stats.log_shapes'
./clp-s s --experimental /mnt/data/archives 'stats.schema_tree'
```

`stats.archives` — one line per archive:

```json
{"archive_id": "...", "num_log_shapes": 1024, "num_vars": 58211}
```

`stats.log_shapes` — one line per log shape, with the number of log messages sharing it:

```json
{"archive_id": "...", "id": 0, "count": 275, "shape": "blk_%block_id.num%_%block_id.gen_stamp%"}
```

`stats.schema_tree` — one line per archive, listing every schema-tree node. Each node has `id`,
`parent_id`, `key`, `type`, `count`, and `children`.

:::{note}
For regular `clp-s` archives, `num_log_shapes` counts log types, and `stats.log_shapes` reports each
log type with a `count` of `null`.
:::
