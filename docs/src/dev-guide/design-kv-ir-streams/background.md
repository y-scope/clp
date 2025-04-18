# Background

To understand the format, it's necessary to briefly understand:

* how clp-s encodes dynamically-structured log events and serializes them into an archive;
* how clp-s parses and encodes unstructured text values.
* auto-generated kv-pairs and how they differ from user-generated kv-pairs.

We explain each below.

## Encoding & storing structured log events in an archive

The processing of encoding a log event can conceptually be broken down into the following steps.

1. [Determine the log event's schema](#1-determine-the-log-events-schema)
2. [Merge the event's schema with the archive's](#2-merge-the-events-schema-with-the-archives)
3. [Create a table for the schema](#3-create-a-table-for-the-schema)
4. [Encode and store the event](#4-encode-and-store-the-event)

### 1. Determine the log event's schema

clp-s first examines the log event's schema--i.e., the key and value type for each kv-pair in the
log event. For example, Figure 1 shows two log events, while Tables 1 & 2 show the schemas for these
log events. clp-s represents this schema as a tree, where each node under the root represents a
unique key and value-type pair. Leaf nodes have primitive types (int, float, boolean, string, null)
and non-leaf nodes are either objects or arrays. The tree for the schema in Table 1 is shown in
Figure 2.

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

:::{card}
| Key       | Value type |
|-----------|------------|
| timestamp | Integer    |
| level     | VarString  |
| message   | ClpString  |
| timers    | Object     |
| - stage_1 | Float      |
| - stage_2 | NullValue  |
+++
**Table 1**: The schema for log event #1 in Figure 1.
:::

:::{card}
| Key       | Value type |
|-----------|------------|
| timestamp | Integer    |
| level     | VarString  |
| message   | ClpString  |
| timers    | Object     |
| - stage_1 | Float      |
| - stage_2 | Float      |
+++
 **Table 2**: The schema for log event #2 in Figure 1.
:::

::::{card}
:::{mermaid}
%%{
  init: {
    "theme": "base",
    "themeVariables": {
      "primaryColor": "#5d00cc",
      "primaryTextColor": "#fff",
      "primaryBorderColor": "transparent",
      "lineColor": "#9580ff",
      "secondaryColor": "#9580ff",
      "tertiaryColor": "#fff"
    }
  }
}%%
flowchart LR
  rootObj("root: <span style='color: orange'>Object</span>")
  timestampInt("timestamp: <span style='color: orange'>Integer</span>")
  levelVarStr("level: <span style='color: orange'>VarString</span>")
  messageClpStr("message: <span style='color: orange'>ClpString</span>")
  timersObj("timers: <span style='color: orange'>Object</span>")
  timersStage1Float("stage_1: <span style='color: orange'>Float</span>")
  timersStage2Null("stage_2: <span style='color: orange'>NullValue</span>")
  
  rootObj --> timestampInt
  rootObj --> levelVarStr
  rootObj --> messageClpStr
  rootObj --> timersObj
  timersObj --> timersStage1Float
  timersObj --> timersStage2Null
:::
+++
**Figure 2**: The schema tree for log event #1 in Figure 1. Each node's label is of the form
"<key>: <type>" and each arrow is from a parent to a child node.
::::

Notice that although both `level` and `mesage` are strings, clp-s assigns them more specific types
based on how it decides to encode them. Table 2 lists clp-s' value types and their meaning.

:::{card}
| Type              | Description                                                                                                                      |
|-------------------|----------------------------------------------------------------------------------------------------------------------------------|
| Integer           | A 64-bit integer                                                                                                                 |
| Float             | An IEEE-754 double-precision floating-point number                                                                               |
| Boolean           | A boolean                                                                                                                        |
| ClpString         | A string that's parsed and encoded using CLP's unstructured-text encoding algorithm                                              |
| DateString        | A string that's parsed and encoded as a timestamp                                                                                |
| VarString         | A string that's encoded using a dictionary                                                                                       |
| UnstructuredArray | An array that's encoded as a JSON string that's been further parsed and encoded using CLP's unstructured-text encoding algorithm |
| StructuredArray   | An array that's encoded natively                                                                                                 |
| Object            | An object                                                                                                                        |
| NullValue         | A null value                                                                                                                     |
+++
**Table 2**: clp-s value types.
:::

### 2. Merge the event's schema with the archive's

For each node in the event's schema tree, clp-s adds it to the archive's schema tree (if it doesn't
already exist) and assigns it a unique ID. In effect, the archive's schema tree is the result of
merging all log events' schemas. Figure 3 shows the archive's schema tree after encoding log event
#1 in Figure 1.

::::{card}
:::{mermaid}
%%{
  init: {
    "theme": "base",
    "themeVariables": {
      "primaryColor": "#5d00cc",
      "primaryTextColor": "#fff",
      "primaryBorderColor": "transparent",
      "lineColor": "#9580ff",
      "secondaryColor": "#9580ff",
      "tertiaryColor": "#fff"
    }
  }
}%%
flowchart LR
  rootObj("<span style='color: cyan'>0</span> root: <span style='color: orange'>Object</span>")
  timestampInt("<span style='color: cyan'>1</span> timestamp: <span style='color: orange'>Integer</span>")
  levelVarStr("<span style='color: cyan'>2</span> level: <span style='color: orange'>VarString</span>")
  messageClpStr("<span style='color: cyan'>3</span> message: <span style='color: orange'>ClpString</span>")
  timersObj("<span style='color: cyan'>4</span> timers: <span style='color: orange'>Object</span>")
  timersStage1Float("<span style='color: cyan'>5</span> stage_1: <span style='color: orange'>Float</span>")
  timersStage2Null("<span style='color: cyan'>6</span> stage_2: <span style='color: orange'>NullValue</span>")

  rootObj --> timestampInt
  rootObj --> levelVarStr
  rootObj --> messageClpStr
  rootObj --> timersObj
  timersObj --> timersStage1Float
  timersObj --> timersStage2Null
:::
+++
**Figure 3**: The archive schema tree after adding log event #1 in Figure 1. Each node's label is of
the form "<ID> <key>: <type>".
::::

### 3. Create a table for the schema

clp-s encodes the log event's schema as a series of node IDs for the leaves of the event's schema
tree. We refer to this set of schema node IDs as the log event's encoded schema. For the log event
in Figure 1, the encoded schema is: `[1, 2, 3, 5, 6]`. Notice that the encoded schema doesn't
include the node IDs of the internal nodes (e.g., `4`) since we can determine those nodes by
traversing upwards from each leaf node; in other words, a leaf node uniquely identifies the complete
hierarchy (JSON path) from the event's root node to the leaf node.

### 4. Encode and store the event

clp-s creates a table for this encoded schema (if one doesn't already exist), where each column
corresponds to a leaf schema node. We refer to these tables as encoded record tables (ERTs). For
each leaf kv-pair in the log event, clp-s encodes it (e.g., using dictionary encoding) and stores
the encoded value in the corresponding ERT column. After encoding the log event in Figure 1, the
corresponding ERT is shown in Table 3.

:::{card}          
| 1             | 2  | 3       | 5     | 6    |
|---------------|----|---------|-------|------|
| 1744618344394 | V0 | L0,V1,2 | 0.753 | null |
+++
**Table 3**: The ERT generated by encoding log event #1 in Figure 1. The value in each column is for
illustration purposes rather than showing the actual value stored. The `info` (ID 2) column contains
a dictionary ID for the variable value. The `message` (ID 3) column contains a dictionary ID for the
log type, followed by variable-dictionary IDs and encoded variable values.
:::

For the second log event, clp-s performs the same procedure resulting in the archive schema tree and
ERT shown in Figure 4 and Table 4. Notice that because log event #2 has a different schema, it adds
a new node to the schema tree and its values are stored in a different ERT.

::::{card}     
:::{mermaid}
%%{
  init: {
    "theme": "base",
    "themeVariables": {
      "primaryColor": "#5d00cc",
      "primaryTextColor": "#fff",
      "primaryBorderColor": "transparent",
      "lineColor": "#9580ff",
      "secondaryColor": "#9580ff",
      "tertiaryColor": "#fff"
    }
  }
}%%
flowchart LR
  rootObj("<span style='color: cyan'>0</span> root: <span style='color: orange'>Object</span>")
  timestampInt("<span style='color: cyan'>1</span> timestamp: <span style='color: orange'>Integer</span>")
  levelVarStr("<span style='color: cyan'>2</span> level: <span style='color: orange'>VarString</span>")
  messageClpStr("<span style='color: cyan'>3</span> message: <span style='color: orange'>ClpString</span>")
  timersObj("<span style='color: cyan'>4</span> timers: <span style='color: orange'>Object</span>")
  timersStage1Float("<span style='color: cyan'>5</span> stage_1: <span style='color: orange'>Float</span>")
  timersStage2Null("<span style='color: cyan'>6</span> stage_2: <span style='color: orange'>NullValue</span>")
  timersStage2Float("<span style='color: cyan'>7</span> stage_2: <span style='color: orange'>Float</span>")
  
  rootObj --> timestampInt
  rootObj --> levelVarStr
  rootObj --> messageClpStr
  rootObj --> timersObj
  timersObj --> timersStage1Float
  timersObj --> timersStage2Null
  timersObj --> timersStage2Float
:::
+++
**Figure 4**: The archive schema tree after adding log event #2 in Figure 1.
::::

:::{card}
| 1             | 2  | 3       | 5     | 7     |
|---------------|----|---------|-------|-------|
| 1744618344499 | V0 | L0,V2,1 | 0.945 | 0.222 |
+++
**Table 4**: The ERT generated by encoding log event #2 in Figure 1.
:::

## Parsing & encoding unstructured text

CLP's algorithm for parsing and encoding unstructured text decomposes a string into:

* the static text of the string, called its log type; and
* variable values.

For example, log event #1's `message` field would be decomposed as shown in Figure 5. Variables in
the log type are replaced with placeholders.

::::{card}
:::{mermaid}
%%{
  init: {
    "theme": "base",
    "themeVariables": {
      "primaryColor": "#5d00cc",
      "primaryTextColor": "#fff",
      "primaryBorderColor": "transparent",
      "lineColor": "#9580ff",
      "secondaryColor": "#9580ff",
      "tertiaryColor": "#fff"
    }
  }
}%%
flowchart TD
    message("<span style='color: cyan'>task_1</span> completed successfully. <span style='color: orange'>2</span> tasks remain.")
    logtype("[] completed successfully. [] 2 tasks remain.")
    var1("<span style='color: cyan'>task_1</span>")
    var2("<span style='color: orange'>2<span>")
    
    message --> logtype
    message --> var1
    message --> var2
:::
+++
Figure 5: How CLP's unstructured-text parsing algorithm decomposes a string. Placeholders in the log
type are represented with `[]`.
::::
    
The algorithm uses a schema file (this "schema" has no relation to the log event's "schema")
containing various regular expressions for parsing and extracting the variable values from the
string. For each variable value, the algorithm encodes it specially to improve compression and/or
query performance. For instance, a string variable may be dictionary-encoded, or a floating-point
value may be encoded using CLP's custom floating-point encoding.

## Autogenerated key-value pairs

The kv-pairs in a log event can typically be divided into two types:

1. *auto-generated* kv-pairs - those inserted automatically by the logging framework.
2. *user-generated* kv-pairs - user-defined kv-pairs inserted explicitly by the user.

For instance, consider the structured Go log printing statement (LPS) in Figure 6 (that uses the
[Zap](https://github.com/uber-go/zap) logging library) and its output in Figure 7.

:::{card}   
```go
logger.Info("failed to fetch URL",
  zap.String("url", url),
  zap.Int("attempt", 3),
  zap.Duration("backoff", time.Second),
)
```
+++
**Figure 6**: An example structured LPS.
:::
    
:::{card} 
```json
{
  "ts":1708161000.123456,
  "level": "info",
  "message": "failed to fetch URL",
  "url": "https://example.com",
  "attempt": 3,
  "backoff": 1000
}
```
+++
**Figure 7**: The output of the LPS in Figure 6.
:::

Comparing the user-defined kv-pairs in Figure 6 and the output in Figure 7, we can see that the
timestamp and level were added automatically by the logging library (Zap). (The key for the
timestamp and log level are Zap configuration options.) In this case, we categorize the user-defined
kv-pairs as user-generated and the log event's timestamp and level as auto-generated. (Even though
the user's logging statement determines the value of the log-level kv-pair, the user doesn't
explicitly insert it into the log event as a kv-pair.)

Differentiating auto-generated and user-generated kv-pairs has a few advantages. First, if we store
auto-generated and user-generated kv-pairs separately, structured logging libraries that generate
kv-ir streams don't need to worry about user-generated keys conflicting with the keys used by the
library. In contrast, a library like Zap doesn't allow a user to specify a kv-pair with a key that's
used for one of the auto-generated kv-pairs. In turn, this means applications that use kv-ir streams
generated by our logging libraries can expect that important auto-generated keys will use consistent
names (e.g., "timestamp" for the log event's timestamp). Second, support for auto-generated kv-pairs
allows us to support more advanced queries on unstructured log events, since unstructured log events
are essentially a set of key-less auto-generated and user-generated values, formatted into a single
string.
