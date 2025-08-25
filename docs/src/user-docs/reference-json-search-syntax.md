# clp-json search syntax

To query JSON logs, CLP currently supports a [variant](#differences-with-kql) of the
[Kibana Query Language (KQL)][kql].

A KQL query is a combination of conditions (predicates) on key-value pairs (kv-pairs). For example:

```
level: ERROR AND attr.ctx: "*conn1*"
```

The query above searches for log events that contain two kv-pairs:

* the key `level` with the value `ERROR`, AND
* the nested key `attr.ctx` with a value that matches the wildcard expression `*conn1*`

## Specification

Below we informally explain all the ways to query log events using KQL.

### Basics

The most basic query is one for a field with a key and a value:

```
key: value
```

To search for a key or value with multiple words, you must quote the key/value with double-quotes
(`"`):

```
"multi-word key": "multi-word value"
```

:::{caution}
Currently, a query that contains spaces is interpreted as a substring search, i.e., it will match
log events that contain the value as a *substring*. In a future version of CLP, these queries will
be interpreted as _exact_ searches unless they include [wildcards](#wildcards-in-values).
:::

:::{note}
Certain characters have special meanings when used in keys or values, so to search for the
characters literally, you must escape them. For a list of such characters, see
[Characters that require escaping](#characters-that-require-escaping).
:::

### Querying nested kv-pairs

If the kv-pair is nested in one or more objects, you can specify the key in one of two ways:

```
parent1.parent2.child: value
```

OR

```
parent1: {parent2: {child: value}}
```

The kv-pair may be nested one or more levels deep.

### Querying auto-generated kv-pairs

If the kv-pair is an auto-generated kv-pair ingested from a kv-ir stream, you can specify that the
key exists within the auto-generated namespace by prefixing the key with `@` as follows:

```
@key: value
```

In the query above, the key's name is `key` and the `@` only indicates the namespace (i.e., `@` is
not considered to be part of the key).

To query for a key named `@key` within the default namespace, the `@` character can be escaped as
follows:

```
\@key: value
```

### Querying file-level metadata kv-pairs

clp-json stores some metadata about each file that is compressed into an archive (e.g., the file's
name). To filter for log events that correspond to some kv-pair in this metadata, you can prefix the
key with `$`:

```
$key: value
```

For example, to query for log events that were compressed from a file whose name was `test.jsonl`:

```
$_filename: "test.jsonl"
```

To query for a key named `$key` in an event, the `$` character can be escaped as follows:

```
\$key: value
```

### Wildcards in values

To search for a kv-pair with *any* value, you can specify the value as a single `*`.

```
key: *
```

To search for a kv-pair where a (string) value contains one or more substrings, you can include `*`
wildcards in the query, where each `*` matches zero or more characters:

```
key: "*partial value*"
```

:::{caution}
Although you can use a single `*` to search for a kv-pair with *any* value, the substring search
syntax above only works for values that are strings.
:::

### Wildcards in keys

To search for a kv-pair with *any* key, you can specify the query in one of two ways:

```
value
```

OR

```
*: value
```

To search for a kv-pair where only some parts of a nested key are known, you can replace the unknown
parts with the `*` wildcard:

```
parent1.*.parent3.child: value
```

:::{note}
CLP does not support queries for partial keys like `parent1*`.
:::

### Numeric comparisons

To search for a kv-pair where the value is a _number_ in some range, you can use numeric comparison
operators in place of the `:`. For example:

```
key > value
```

The following comparison operators are supported:

* `>` - the kv-pair's value is greater than the specified value
* `>=` - the kv-pair's value is greater than or equal to the specified value
* `<` - the kv-pair's value is less than the specified value
* `<=` - the kv-pair's value is less than or equal to the specified value

:::{note}
There is no `=` operator since `:` functions as an equality operator.
:::

### Querying array values

To search for a kv-pair where the value is in an array, the syntax is the same as searching for a
nested kv-pair. For example, the query below...

```
parent1: {parent2: {child: value}}
```

...would match the log event below:

```json
{"parent1": [{"parent2": {"child": "value"}}]}
```

:::{caution}
By default, CLP does not support queries for array kv-pairs where only part of the key is known. In
other words, the key must either be a wildcard (`*`) or it must contain no wildcards.

Archives compressed using the `--structurize-arrays` flag *do not* have this limitation.
:::

### Complex queries

You can search for one or more kv-pairs by combining them with boolean algebra. For example:

```text
key1: value1 AND (key2: valueA OR key2: valueB) AND NOT key3: value3
```

There are three supported boolean operators:

* `AND` - the expressions on _both_ sides of the operator must be true.
* `OR` - the expressions on _either_ side of the operator must be true.
* `NOT` - the expression after the operator must _not_ be true.

You can use parentheses (`()`) to apply an operator to a group of expressions.

### Characters that require escaping

Keys containing the following literal characters must escape the characters using a `\` (backslash):

* `\`
* `"`
* `.`
* `*`

Furthermore, keys that _start_ with the following literal characters must escape the characters
using a `\` (backslash):

* `@`
* `$`
* `!`
* `#`

Values containing the following literal characters must escape the characters using a `\`
(backslash):

* `\`
* `"`
* `?`
* `*`

_Unquoted_ keys or values containing the following literal characters must also escape the
characters using a `\` (backslash):

* `(`
* `)`
* `:`
* `<`
* `>`
* `{`
* `}`

### Supported escape sequences

Keys and values can represent Unicode codepoints using the `\uXXXX` escape sequence, where each `X`
is a hexadecimal character.

Keys and values also support the following escape sequences to represent control characters:

* `\b`
* `\f`
* `\n`
* `\r`
* `\t`

:::{note}
The escape sequences in this section are described verbatim and don't need an extra backslash to
escape the backslash at the beginning of each sequence.
:::

## Examples

**Search for log events that contain a specific key-value pair:**

```
id: 22149
```

**Search for ERROR log events containing a substring:**

```
level: ERROR AND message: "*job*"
```

**Search for FATAL log events containing the substring "container":**

```
level: FATAL OR *: *container*
```

**Search for log events where the value of a nested key is in some range:**

```
job.stats.latency > 0.5 AND job.stats.latency <= 5
```

**Search for log events where part of the key is unspecified:**

```
job.*.status: FAILED
```

**Search for log events where the value of any child key is "STOPPED":**

```
job.*: STOPPED
```

## Differences with KQL

There are a few notable differences between CLP's search syntax and KQL:

* CLP allows a value to contain leading wildcards, by default, whereas they must be explicitly
  enabled when using KQL with Elasticsearch.
* CLP doesn't currently support fuzzy matches (e.g., misspellings) for a value, whereas KQL on
  Elasticsearch may perform a fuzzy match depending on how the kv-pair was ingested.
* CLP will perform a substring search if the query value contains wildcards or includes spaces,
  whereas KQL on Elasticsearch may perform a fuzzy match (equivalent to a substring search)
  depending on how the kv-pair was ingested.
* CLP doesn't support the following _shorthand_ syntax for matching one or more values with the same
  key: `key: (value1 or value2)`.
  * In CLP, this query can be written as `key: value1 OR key: value2`.
* CLP doesn't support unquoted multi-word queries (e.g. `key: word1 word2`), whereas KQL allows it
  for queries that only contain a single predicate.
* CLP doesn't support using comparison operators on strings, IP addresses, or timestamps whereas
  KQL does.
* When querying for multiple kv-pairs in an array, CLP does not guarantee that all kv-pairs are in
  the same object, whereas KQL does.
  * For example, in CLP, the query `a: {"b": 0, "c": 0}` will match log events like

    ```json
    {"a": [{"b": 0}, {"c": 0}]}
    ```

    and

    ```json
    {"a": [{"b": 0, "c": 0}]}
    ```

    Whereas with KQL, the query would only match the second log event.

[kql]: https://www.elastic.co/guide/en/kibana/current/kuery-query.html
