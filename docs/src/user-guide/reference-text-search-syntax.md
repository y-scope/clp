# Text-log search syntax

To search text logs, CLP currently supports wildcard queries. A wildcard query is a query where:

* `*` matches zero or more characters
* `?` matches any single character

To search for a wildcard literally, you can escape it using a backslash (`\`). To search for a
backslash literally, you can escape it with another backslash.

By default, CLP treats queries as substring searches (alike `grep`). So the query
`container * failed` is interpreted internally as `*container * failed*` (with prefix and suffix
`*`).

:::{tip}
Queries where words contain wildcards are generally[^1] slower than queries where the words are
separate from any wildcards. For example, the query `ERROR c*4 FAILED!` will likely take longer to
complete than the query ` ERROR container_4 FAILED!` (assuming they match the same log events).
:::

## Quoting in the UI

When searching from the UI, you may quote your query with double quotes (`"`). To search for a
double quote character literally, you can escape it using a backslash (`\`). To search for a
backslash literally, you can escape it with another backslash.

:::{note}
Escaped wildcards within a _quoted_ query string **don't** need to be escaped again.
:::

## Examples

**Search for log events for failed containers:**

```
container * failed
```

_Note that a wildcard query is a substring search, so all non-wildcard characters (like spaces) are
matched literally._

**Search for log events for containing a 3-digit number that starts and ends with `3`:**

```
 3?3 
```

_Note that because the query is surrounded by spaces, it will be interpreted internally as
`* 3?3 *`._

**Search for the literal substring ` formula: \x * \y ` (which requires escapes):**

```
 formula: \\x \* \\y
```

[^1]: A "word" is any contiguous group of non-delimiter characters. A delimiter is a character that
CLP uses to split-up (tokenize) a log event. By default, these delimiters are any non-alphanumeric
character except `+`, `-`, `.`, and `_`; but users can
[configure](reference-unstructured-schema-file) these delimiters if they wish.
