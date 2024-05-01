# JSON-log search syntax

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

To search for a key or value with multiple words, you should quote the key/value:

```
"multi-word key": "multi-word value"
```

Queries for keys or values with the following literal characters must escape the characters using a
`\` (backslash): `\`, `(`, `)`, `:`, `<`, `>`, `"`, `*`, `{`, `}`.

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

### Wildcards queries for values

To search for a kv-pair with *any* value, you can specify the value `*`

```
key: *
```

To search for a kv-pair where the value contains a substring, you can specify the value as a
wildcard expression, where the wildcard `*` which matches zero or more characters:

```
key: "*partial value*"
```

:::{caution}
Wildcard expressions only work for kv-pairs where the value is a string. 
:::

:::{caution}
CLP doesn't currently support the `?` wildcard (which matches any single character) _except_ in
values containing multiple words. This limitation will be addressed in a future version of CLP. 
:::

### Wildcard queries for keys

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
CLP does not currently support queries for array kv-pairs where only part of the key is known. In
other words, the key must either be a wildcard (`*`) or it must contain no wildcards.
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

## Examples

**Search for log events that contain a specific key-value pair:**

```
id: 22149
```

**Search for ERROR log events containing a substring:**

```
level: ERROR AND message: "*job*"
```

**Search for FATAL or ERROR log events:**

```
level: FATAL OR level: ERROR
```

(differences-with-kql)=
## Differences with KQL

There are a few notable differences between CLP's search syntax and KQL:

* CLP allows a value to contain leading wildcards.
* CLP does not currently support fuzzy matches (e.g., misspellings) on a value whereas KQL on
  Elasticsearch may perform a fuzzy match depending on how the kv-pair was ingested.
* We don't support the following _shorthand_ syntax for matching one or more values with the same
  key: `key: (value1 or value2)`
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
    
    Whereas with KQL, it would only match the second document.

[kql]: https://www.elastic.co/guide/en/kibana/current/kuery-query.html
