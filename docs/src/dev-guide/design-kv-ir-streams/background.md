# Background

To understand the KV-IR stream format, we first need to review the following:

* [How clp-s compresses log events](#clp-s-compression), since the process for KV-IR streams is
  similar but makes a different trade-off between resource usage, compression ratio, and search
  performance.
* [How clp-s parses and encodes unstructured text values](#parsing--encoding-unstructured-text).

We discuss each below.

## clp-s compression

At a high-level, [clp-s](../../user-guide/core-clp-s.md) compresses log events into what we call
archives. Depending on the configured size-threshold for each archive, a set of log events may be
compressed into one or more archives. The archive format is designed so that each archive is
self-contained and independent, allowing archives to be searched concurrently.

To compress a log event into an archive, clp-s needs to do the following:

1. [Compute the event's schema](#computing-a-log-events-schema)
2. [Encode the event's schema](#encoding-log-event-schemas)
3. [Encode the event's values](#encoding-log-event-values)
4. [Store the event's encoded values](#storing-encoded-values)

Finally, when all events for an archive have been processed, clp-s needs to serialize and
[write the archive's data structures](#writing-archives-to-disk). The goal of this process is to
transform the log events into a form that's more compact to store and faster to search.

:::{tip}
To learn more about clp-s, check out the original [research paper][clp-s-paper].
:::

### Computing a log event's schema

A log event's schema is the set of *key* and *value-type* pairs for each KV pair in the log event.
To compute an event's schema, clp-s iterates over every KV pair in the log event to:

* determine the clp-s value-type that should be assigned to the KV pair.
* build a tree representation of the schema---what we call a schema tree.

Consider the example log events in [Figure 1](#figure-1), and their schemas in [Table 1](#table-1)
and [Table 2](#table-2). clp-s' value types, including those used in the schema, are listed in
[Table 3](#table-3).

clp-s assigns a type to a value based on the value's "abstract" type (i.e., whether it's an integer,
float, boolean, string, object, null, or array) and how the value should be encoded. For some
abstract types, clp-s only has one way of encoding it, so it assigns the corresponding clp-s
type---e.g., the integer corresponding to the `timestamp` key. For other abstract types, clp-s can
encode the value in multiple ways, so it assigns the clp-s type where the encoded value will result
in a good trade-off between compactness and efficient searches---e.g., the strings corresponding to
the `level` and `message` keys use different clp-s types.

Once clp-s assigns a type to a value, it can add it to the log event's schema tree. Except for the
root, each node in a schema tree represents a unique key and value-type pair from the schema. For
instance, the tree for the schema in [Table 1](#table-1) is shown in [Figure 2](#figure-2). Since
the tree represents the structure of a structured log event, each internal (non-leaf) node will
always correspond to an `Object` or `StructuredArray`, while the leaf nodes will correspond to
values with primitive types (since `UnstructuredArray` values are encoded as JSON strings, they are
primitives from the perspective of a schema tree). Accordingly, the root node represents the event
object itself, and has no key.

(figure-1)=
:::{card}

```json lines
{
  "timestamp": 1744618344394,
  "level": "info",
  "message": "task_1 completed successfully. 2 task(s) remain.",
  "timers": {
    "stage_1": 0.753,
    "stage_2": null
  }
}

{
  "timestamp": 1744618344499,
  "level": "info",
  "message": "task_2 completed successfully. 1 task(s) remain.",
  "timers": {
    "stage_1": 0.945,
    "stage_2": 0.222
  }
}
```

+++
**Figure 1**: Two JSON log events.
:::

(table-1)=
:::{card}

| Key            | clp-s value-type |
|----------------|------------------|
| timestamp      | Integer          |
| level          | VarString        |
| message        | ClpString        |
| timers         | Object           |
| timers.stage_1 | Float            |
| timers.stage_2 | NullValue        |

+++
**Table 1**: The schema for log event &#35;1 in [Figure 1](#figure-1). Nested keys are represented
with dot notation. The value types are described in [Table 3](#table-3).
:::

(table-2)=
:::{card}

| Key            | clp-s value-type |
|----------------|------------------|
| timestamp      | Integer          |
| level          | VarString        |
| message        | ClpString        |
| timers         | Object           |
| timers.stage_1 | Float            |
| timers.stage_2 | Float            |

+++
**Table 2**: The schema for log event &#35;2 in [Figure 1](#figure-1).
:::

(table-3)=
:::{card}

| clp-s value-type  | Description                                                                                             | Node type |
|-------------------|---------------------------------------------------------------------------------------------------------|-----------|
| Integer           | A 64-bit integer                                                                                        | Leaf      |
| Float             | A floating-point number                                                                                 | Leaf      |
| Boolean           | A boolean                                                                                               | Leaf      |
| VarString         | A string without whitespace                                                                             | Leaf      |
| DateString        | A string representing a timestamp                                                                       | Leaf      |
| ClpString         | A string containing whitespace, parsed into an [encoded text AST](#parsing--encoding-unstructured-text) | Leaf      |
| NullValue         | A null value                                                                                            | Leaf      |
| UnstructuredArray | An array that's serialized as a JSON string                                                             | Leaf      |
| Object            | An object                                                                                               | Internal  |
| StructuredArray   | An array                                                                                                | Internal  |

+++
**Table 3**: clp-s value types.
:::

(figure-2)=
::::{card}
:::{mermaid}
%%{
  init: {
    "theme": "base",
    "themeVariables": {
      "primaryColor": "#0066cc",
      "primaryTextColor": "#fff",
      "primaryBorderColor": "transparent",
      "lineColor": "#007fff",
      "secondaryColor": "#007fff",
      "tertiaryColor": "#fff"
    }
  }
}%%
flowchart LR
  rootObj("&lt;Root&gt;: <span style='color: #97ff00'>Object</span>")
  messageClpStr("&quot;message&quot;: <span style='color: #97ff00'>ClpString</span>")
  levelVarStr("&quot;level&quot;: <span style='color: #97ff00'>VarString</span>")
  timersObj("&quot;timers&quot;: <span style='color: #97ff00'>Object</span>")
  timersStage1Float("&quot;stage_1&quot;: <span style='color: #97ff00'>Float</span>")
  timersStage2Null("&quot;stage_2&quot;: <span style='color: #97ff00'>NullValue</span>")
  timestampInt("&quot;timestamp&quot;: <span style='color: #97ff00'>Integer</span>")

  rootObj --> timestampInt
  rootObj --> levelVarStr
  rootObj --> timersObj
  timersObj --> timersStage1Float
  timersObj --> timersStage2Null
  rootObj --> messageClpStr
:::
+++
**Figure 2**: The schema tree for log event &#35;1 in [Figure 1](#figure-1). Each node's label is of
the form `"<key>": <type>`, except for the root which doesn't have an explicit name. Each arrow is
from a parent to a child node.
::::

### Encoding log event schemas

To compactly encode each event's schema in an archive, clp-s represents each schema with a set of
integer IDs corresponding to nodes of an archive-level schema tree. This archive-level schema tree
is built, in part, by merging all events' schema trees and assigning a unique ID to each node. An
event's schema can then be encoded as the IDs of its *leaf* nodes within the tree, since the leaf
nodes are sufficient to rebuild the event's tree by traversing from the leaves to the root. For
instance, [Figure 3](#figure-3) shows the schema tree after adding the example logs
([Figure 1](#figure-1)) to the tree. The events' schema trees have been merged under the
`<Default namespace>` node. Referencing the leaf node IDs, the schema for event &#35;1 can be
encoded as `[3, 4, 6, 7, 9]`, corresponding to the schema's leaf nodes.

As [Figure 3](#figure-3) shows, the archive-level schema tree uses different *namespaces* to store
more than just the KV pairs that appear *in* the event. For instance, the `Metadata` namespace
contains metadata KV pairs like the log event's index in the archive. The `Default` namespace
contains the KV pairs that aren't specific to a special namespace, which in the case of Figure 3,
are the KV pairs that appear in the example log events. As we'll see in future docs, namespaces
also allow clp-s to compress log events that contain namespaces themselves.

To merge an event's schema tree with the archive-level schema tree, clp-s iterates over each pair of
nodes---one from each tree:

* If the nodes have the same key and value-type, and all of their predecessor nodes have matching
  key and value-type pairs, clp-s merges the nodes in the resulting tree.
* Otherwise, both nodes are added to the resulting tree, and each is assigned a unique integer ID.

<!-- markdownlint-disable MD013 -->
(figure-3)=
::::{card}
:::{mermaid}
%%{
  init: {
    "theme": "base",
    "themeVariables": {
      "primaryColor": "#0066cc",
      "primaryTextColor": "#fff",
      "primaryBorderColor": "transparent",
      "lineColor": "#007fff",
      "secondaryColor": "#007fff",
      "tertiaryColor": "#fff"
    }
  }
}%%
flowchart LR
  root("<span style='color: #ffbe00'>-1</span> &lt;Root&gt;")
  metadataNamespaceRoot("<span style='color: #ffbe00'>0</span> &lt;Metadata namespace&gt;: <span style='color: #97ff00'>Metadata</span>")
  logEventIdxInt("<span style='color: #ffbe00'>1</span> &quot;log_event_idx&quot;: <span style='color: #97ff00'>Integer</span>")
  defaultNamespaceRootObj("<span style='color: #ffbe00'>2</span> &lt;Default namespace&gt;: <span style='color: #97ff00'>Object</span>")
  messageClpStr("<span style='color: #ffbe00'>3</span> &quot;message&quot;: <span style='color: #97ff00'>ClpString</span>")
  levelVarStr("<span style='color: #ffbe00'>4</span> &quot;level&quot;: <span style='color: #97ff00'>VarString</span>")
  timersObj("<span style='color: #ffbe00'>5</span> &quot;timers&quot;: <span style='color: #97ff00'>Object</span>")
  timersStage1Float("<span style='color: #ffbe00'>6</span> &quot;stage_1&quot;: <span style='color: #97ff00'>Float</span>")
  timersStage2Null("<span style='color: #ffbe00'>7</span> &quot;stage_2&quot;: <span style='color: #97ff00'>NullValue</span>")
  timersStage2Float("<span style='color: #ffbe00'>8</span> &quot;stage_2&quot;: <span style='color: #97ff00'>Float</span>")
  timestampInt("<span style='color: #ffbe00'>9</span> &quot;timestamp&quot;: <span style='color: #97ff00'>Integer</span>")

  root --> metadataNamespaceRoot
  metadataNamespaceRoot --> logEventIdxInt
  root --> defaultNamespaceRootObj
  defaultNamespaceRootObj --> timestampInt
  defaultNamespaceRootObj --> levelVarStr
  defaultNamespaceRootObj --> timersObj
  timersObj --> timersStage1Float
  timersObj --> timersStage2Null
  timersObj --> timersStage2Float
  defaultNamespaceRootObj --> messageClpStr
:::
+++
**Figure 3**: The archive's schema tree after adding the log events from [Figure 1](#figure-1). Each
node's label is of the form `<ID> <key>: <type>` except for the namespace nodes which don't have an
explicit name, and the root which has neither an explicit name nor type.
::::
<!-- markdownlint-enable MD013 -->

### Encoding log event values

For each log event, clp-s encodes each value using an encoding method for the value's specific type.
The goal of each method is to deduplicate any repetitive information (e.g., deduplicating repeated
`VarString` values with a dictionary) and then represent the value with a 64-bit integer.
[Table 4](#table-4) lists how clp-s encodes each value type. Most value types are encoded
conventionally with the following exceptions:

* For the values encoded as dictionary IDs, clp-s simply stores the value in a dictionary and maps
  it to a unique integer ID.
* For `ClpString` values, clp-s encodes each component separately.
* For `NullValue` values, clp-s doesn't need to encode anything since they don't need to be stored
  explicitly---a `NullValue` leaf node already indicates that the corresponding column of the ERT is
  null.

(table-4)=
:::{card}

| clp-s value-type            | Encoding                                                    |
|-----------------------------|-------------------------------------------------------------|
| Integer                     | 8-byte integer                                              |
| Float                       | 8-byte IEEE-754 double-precision float                      |
| Boolean                     | 1-byte integer                                              |
| VarString                   | 8-byte dictionary ID                                        |
| DateString                  | 8-byte epoch timestamp & 8-byte format string dictionary ID |
| ClpString                   | *See below*                                                 |
| --> Format string           | 8-byte dictionary ID                                        |
| --> Encoded variable values | Collection of 8-byte integers                               |
| --> String variable values  | Collection of 8-byte dictionary IDs                         |
| UnstructuredArray           | Same as ClpString                                           |
| NullValue                   | N/A                                                         |

+++
**Table 4**: How clp-s encodes each of its leaf node value types.
:::

clp-s' two array types are used to encode arrays with different characteristics. `StructuredArray`
values are similar to `Object` values in that all of their elements will be added to the schema
tree. Accordingly, this type is more appropriate for encoding arrays whose elements don't change
types significantly between log events; otherwise, the schema tree would be significantly larger.
For other arrays, the `UnstructuredArray` type is more appropriate---since it's encoded as a JSON
string, its elements won't be added to the tree. Nonetheless, values within these arrays can still
be searched.

### Storing encoded values

clp-s stores a log event's encoded values in a table corresponding to its schema, with one column
for each node in the schema. We refer to this table as an encoded record table (ERT). By grouping
events with the same schema into an ERT, clp-s avoids redundantly storing the schema per event
(unlike, for example, JSON). In addition, ERTs are efficient to search since all columns store
integers.

### Writing archives to disk

To write an archive's data structures to disk, clp-s serializes them and writes them to one or more
general-purpose compression streams. Applying general-purpose compression allows us to mitigate some
of the inefficient encodings (e.g., encoding `Boolean` values as integers) used to maintain
efficient search performance. For some data structures, like dictionaries, clp-s writes them to disk
as they are built; yet for other data structures, like the ERTs, clp-s buffers them in memory until
the archive is complete.

## Parsing & encoding unstructured text

clp-s uses [clp](../../user-guide/core-unstructured/clp.md)'s algorithm to parse and encode
unstructured text. Unstructured text is a string that contains zero or more variable values
interspersed with non-variable (static) text. For example, in [Figure 1](#figure-1), log event
&#35;1's `message` value is unstructured text containing the variable values `task_1` and `2`. At a
high-level, clp's algorithm uses a set of user-defined regular expressions to match each variable
value in the unstructured text, decomposing the text into:

* a format string---i.e., the unstructured text with variable values replaced with placeholders.
* string variable values.
* encoded variable values---i.e., variable values which have been encoded as 64-bit integers.

Collectively, we refer to these three components as an *encoded text AST*. For instance, log event
&#35;1's `message` value would be decomposed into the following encoded text AST:

* Format string: `\x12 completed successfully. \x11 task(s) remain.`
  * `\x12` and `\x11` are variable placeholders representing string and integer variables,
    respectively.
* String variable values: `["task_1"]`
* Encoded variable values: `[1]`

:::{note}
The clp codebase refers to an encoded text AST's string variable values as "dictionary variables,"
since they're typically stored in a dictionary. This may change as we update the codebase.
:::

:::{tip}
To learn more about clp, check out the original [research paper][clp-paper].
:::

[clp-paper]: https://www.usenix.org/system/files/osdi21-rodrigues.pdf
[clp-s-paper]: https://www.usenix.org/system/files/osdi24-wang-rui.pdf
