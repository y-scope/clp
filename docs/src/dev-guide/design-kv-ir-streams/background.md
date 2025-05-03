# Background

To understand the KV-IR stream format, we first need to review the following:

* [How clp-s compresses log events](#clp-s-compression), since the process for KV-IR streams is
  similar but makes a different trade-off between resource usage, compression ratio, and search
  performance.
* [How clp-s parses and encodes unstructured text values](#parsing--encoding-unstructured-text).

We discuss each below.

## clp-s compression

At a high-level, clp-s compresses log events into what we call archives. Depending on the configured
size-threshold for each archive, a set of log events may be compressed into one or more archives.
In addition, the format of an archive is designed so that each archive is independent of other
archives, meaning that different archives can be searched concurrently.

To compress a log event into an archive, clp-s needs to do the following:

1. [Compute the event's schema](#computing-a-log-events-schema)
2. [Encode the event's schema](#encoding-log-event-schemas)
3. [Encode and store the event's values](#encoding--storing-event-values)
4. [Serialize and write the archive's data structures](#writing-archives-to-disk)

The goal of this process is to transform the log events into a form that's more compact to store and
faster to search.

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
type--e.g., the integer corresponding to the `timestamp` key. For other abstract types, clp-s can
encode the value in multiple ways, so it assigns the clp-s type where the encoded value will result
in a good trade-off between compactness and efficient searches---e.g., the strings corresponding to
the `level` and `message` keys use different clp-s types.

Once clp-s assigns a type to a value, it can add it to the log event's schema tree. Except for the
root, each node in a schema tree represents a unique key and value-type pair from the schema. For
instance, the tree for the schema in [Table 1](#table-1) is shown in [Figure 2](#figure-2). Since
the tree represents the structure of a structured log event, the internal (non-leaf) nodes will
always correspond to `Object`s or `StructuredArray`s, while the leaf nodes will correspond to values
with primitive types (since `UnstructuredArray` values are encoded as JSON strings, they are
primitives from the perspective of a schema tree) . Accordingly, the root node represents the event
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
**Table 1**: The schema for log event #1 in [Figure 1](#figure-1). Nested keys are represented with
dot notation. The value types are described in [Table 3](#table-3).
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
 **Table 2**: The schema for log event #2 in [Figure 1](#figure-1).
:::

(table-3)=
:::{card}

| clp-s value-type  | Description                                                                                            | Node type |
|-------------------|--------------------------------------------------------------------------------------------------------|-----------|
| Integer           | A 64-bit integer                                                                                       | Leaf      |
| Float             | A floating-point number                                                                                | Leaf      |
| Boolean           | A boolean                                                                                              | Leaf      |
| VarString         | A string without whitespace                                                                            | Leaf      |
| DateString        | A string representing a timestamp                                                                      | Leaf      |
| ClpString         | A string containing whitespace, parsed into an [encoded text AST](#parsing--encoding-unstructured-text) | Leaf      |
| UnstructuredArray | An array that's serialized as a JSON string                                                             | Leaf      |
| NullValue         | A null value                                                                                           | Leaf      |
| Object            | An object                                                                                              | Internal  |
| StructuredArray   | An array                                                                                               | Internal  |

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
  rootObj("root: <span style='color: #97ff00'>Object</span>")
  timestampInt("timestamp: <span style='color: #97ff00'>Integer</span>")
  levelVarStr("level: <span style='color: #97ff00'>VarString</span>")
  messageClpStr("message: <span style='color: #97ff00'>ClpString</span>")
  timersObj("timers: <span style='color: #97ff00'>Object</span>")
  timersStage1Float("stage_1: <span style='color: #97ff00'>Float</span>")
  timersStage2Null("stage_2: <span style='color: #97ff00'>NullValue</span>")

  rootObj --> timestampInt
  rootObj --> levelVarStr
  rootObj --> messageClpStr
  rootObj --> timersObj
  timersObj --> timersStage1Float
  timersObj --> timersStage2Null
:::
+++
**Figure 2**: The schema tree for log event #1 in [Figure 1](#figure-1). Each node's label is of the
form `<key>: <type>`, and each arrow is from a parent to a child node.
::::

### Encoding log event schemas

To compactly encode the schemas of the events in an archive, clp-s maintains an archive-level schema
tree that merges all events' schema trees; each event's schema can then be uniquely encoded as
an array of identifiers for the relevant leaf nodes of the archive's tree. (The leaf node IDs are
sufficient since we can determine the predecessor nodes by traversing to the root from the leaves.)
For instance, [Figure 3](#figure-3) shows the schema tree after adding the example logs
([Figure 1](#figure-1)) to the tree. Referencing the leaf node IDs, the schema for event #1 can be
encoded as `[1, 2, 3, 5, 7]`. To merge an event's schema tree with the archive's tree, clp-s
iterates over each pair of nodes---one from each tree:

* If the nodes have the same key and value-type, and all of their predecessor nodes have matching
  key and value-type pairs, clp-s merges the nodes in the resulting tree.
* Otherwise, both nodes are added to the resulting tree, and each is assigned a unique integer ID.

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
  rootObj("<span style='color: #ffbe00'>0</span> root: <span style='color: #97ff00'>Object</span>")
  timestampInt("<span style='color: #ffbe00'>1</span> timestamp: <span style='color: #97ff00'>Integer</span>")
  levelVarStr("<span style='color: #ffbe00'>2</span> level: <span style='color: #97ff00'>VarString</span>")
  messageClpStr("<span style='color: #ffbe00'>3</span> message: <span style='color: #97ff00'>ClpString</span>")
  timersObj("<span style='color: #ffbe00'>4</span> timers: <span style='color: #97ff00'>Object</span>")
  timersStage1Float("<span style='color: #ffbe00'>5</span> stage_1: <span style='color: #97ff00'>Float</span>")
  timersStage2Null("<span style='color: #ffbe00'>6</span> stage_2: <span style='color: #97ff00'>NullValue</span>")
  timersStage2Float("<span style='color: #ffbe00'>7</span> stage_2: <span style='color: #97ff00'>Float</span>")

  rootObj --> timestampInt
  rootObj --> levelVarStr
  rootObj --> messageClpStr
  rootObj --> timersObj
  timersObj --> timersStage1Float
  timersObj --> timersStage2Null
  timersObj --> timersStage2Float
:::
+++
**Figure 3**: The archive's schema tree after adding the log events from [Figure 1](#figure-1). Each
node's label is of the form `<ID> <key>: <type>`.
::::

### Encoding & storing event values

For each log event, clp-s uses a variety of encoding methods to compactly encode its values before
storing them in a table corresponding to its schema. We refer to this table as an encoded record
table (ERT). As we'll see below, clp-s uses a different encoding method for each of its value types,
with the goal of each encoding method being to deduplicate any repetitive information (e.g.,
deduplicating repeated `VarString`s with a dictionary) and then represent the value with a 64-bit
integer. For each encoded value, clp-s stores it in the value's corresponding ERT column. ERTs are
efficient to search since all the values are integers; and by grouping events with the same schema
into an ERT, clp-s essentially deduplicates the event's schema.

clp-s encodes each leaf-node value type as follows:

* `Integer`s are encoded natively.
* `Float`s are encoded using the IEEE-754 double-precision format.
* `Boolean`s are encoded as an integer.
* `VarString`s are encoded using dictionary-encoding---i.e., a dictionary that maps string values to
  unique integer IDs, and the encoded value for a string value is its corresponding dictionary ID.
* `DateString`s are encoded as an epoch timestamp and a dictionary-encoded format string.
* `ClpString`s are encoded as follows:
  * the format string is dictionary-encoded.
  * the encoded variable values are encoded natively.
  * the string variable values are dictionary-encoded.
* `UnstructuredArray`s are converted to EncodedTextAsts are then encoded similar to `ClpString`s.
* `NullValue`s are encoded as the integer `0`.

`VarString`s share a dictionary with the string variable values from `ClpString`s and
`UnstructuredArray`s, while the format strings from `DateString`s, `ClpString`s, and
`UnstructuredArray`s use separate dictionaries, each.

clp-s' two array types are used to encode arrays with different characteristics. `StructuredArray`
values are similar to `Object` values in that all of their elements will be added to the schema
tree. Accordingly, this type is more appropriate for encoding arrays whose elements don't change
types significantly between log events; otherwise, the schema tree would be significantly larger.
For other arrays, the `UnstructuredArray` type is more appropriate---since it's encoded as a JSON
string, its elements won't be added to the tree. Nonetheless, values within these arrays can still
be searched.

### Writing archives to disk

To write an archive's data structures to disk, clp-s serializes them and writes them to one or more
general-purpose compression streams. Applying general-purpose compression allows us to mitigate some
of the inefficient encodings (e.g., encoding `Boolean`s as integers) used to maintain efficient
search performance. For some data structures, like dictionaries, clp-s writes them to disk as they
are built; yet for other data structures, like the ERTs, clp-s buffers them in memory until the
archive is complete.

## Parsing & encoding unstructured text

clp-s uses clp's algorithm to parse and encode unstructured text. Unstructured text is a string that
contains zero or more variable values interspersed with non-variable (static) text. For example, in
[Figure 1](#figure-1), log event #1's `message` value is unstructured text containing the variable
values `task_1` and `2`. At a high-level, the clp algorithm uses a set of user-defined regular
expressions to match each variable value in the unstructured text, decomposing the text into:

* a format string---i.e., the unstructured text with variable values replaced with placeholders;
* string variable values; and
* encoded variable values---i.e., variable values which have been encoded as 64-bit integers.

Collectively, we refer to these three components as an _encoded text AST_. For instance, log event
#1's `message` value would be decomposed into the following encoded text AST:

* Format string: `\x12 completed successfully. \x11 task(s) remain.`
* `\x12` and `\x11` are variable placeholders representing string and integer variables,
  respectively.
* String variable values: `["task_1"]`
* Encoded variable values: `[1]`

:::{note}
The clp codebase refers to an encoded text AST's string variable values as "dictionary variables,"
since they're typically stored in a dictionary. This may change as we update the codebase.
:::
