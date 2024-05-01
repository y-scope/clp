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
Including delimiters (e.g. spaces, colons, etc.) at the beginning and end of your query can improve
CLP's search performance.

If your query begins/ends with a token, since CLP implicitly adds wildcards to each end of the
query, that token will have wildcards attached to it. Generally, queries for tokens containing
wildcards are slower than queries for tokens that don't contain wildcards.
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
