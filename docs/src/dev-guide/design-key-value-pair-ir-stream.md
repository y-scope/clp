# Key-Value Pair IR Stream

Key-Value Pair IR Stream is a new intermediate representation format for CLP-S that supports both 
semi-structured logs and unstructured logs. Here we will walk through the technical details of this
key step forward for CLP-S. At a high level, we use a streaming format to represent the 
semi-structured data. We call each section for the stream an IR unit. These units come in four 
types; a metadata unit, schema tree growth units, log event units, and an end of stream unit. Here 
we will talk through this streaming format, how this intermediary representations works with CLP-S 
archive format, and compare it to CLP’s IRv1. If you are unfamialr with CLP's IRv1 format please 
checkout [this](#background-in-irv1) section below.

## Introduction

As outlined in [Uber's blog post][uber-blog] from 2022, the CLP IR stream format is a lightweight
serialization format designed for log streaming compression. However, due to its design constraints,
this IR stream format is primarily suited for simple, unstructured logs, such as raw text logs,
which typically consist of only a timestamp field and a log message field. Consequently,
structured logs, such as JSON logs, cannot be efficiently serialized using this format.

[CLP-S][clp-s-osdi] is an extension of the CLP's design principle but applied to semi-structured logs such as 
JSON. It uses the key value structure of JSON entries so that a schema is a unique set of keys and 
the values associated with those keys are the variables. Each schema can be represented by a tree. 
When a unique key name and primitive type pair is encountered, we add a new leaf node to the tree. 
The internal nodes of the tree are any surrounding objects. We merge these individual trees 
together to form a merged parse tree which we’ll refer to as the schema tree. If two keys in 
different schemas share a key name, but not a type, we add that new node to the tree. Since a set of key and type pairs make a schema, more than one log event can share the same schema and just have seperate values correpsonding to the keys. We collect the unique schemas forming a schema map. Like in CLP these
 values can be further deduplicated using dictionaries. You can see an overall CLP-S workflow in 
 Figure 1 and you can read further details here; 
 [OSDI24 - wang et. al](https://www.usenix.org/conference/osdi24/presentation/wang-rui). 

![image info](./images/design-IRV2_uSlope_paper_visualization.png)
<p style="text-align:center;"><strong>Figure 1</strong>: Figure 7 from μSlope: High Compression and
 Fast Search on Semi-Structured Logs by Wang et al.</p>

To see how a a schema tree and schema map are formed from a few example log messages you can look here.(TODO: Link to Example Log Events, CLP-S Schema Tree)

Building upon the principles of the existing IR stream format and drawing inspiration from CLP-S, 
we have developed a new IR format: we call Key-Value Pair IR Stream format. This new format 
enhances the original design by efficiently supporting key-value pair serialization, thereby 
addressing the limitations of the previous IR format.

This new IR format has been successfully deployed in production environments to serialize real-
world log events for cost reduction. These log events originate from diverse sources, including:
- Application and services logs in datacenters.
- User, application, and FSD (full self-driving) operational logs on electrical vehicles on the 
  road.

## Design Overview

Our goal for Key-Value Pair IR Stream (KV Pair IR) is to break CLP-S into phases like we did for 
CLP so that we can still achieve a substantial amount of initial compression, but leave the heavy 
resource intensive deduplication of variable values for later processing. Like IRv1, KV Pair IR 
enables the lossless compression of logs on an entry by entry basis. KV Pair IR is a superset of 
IRv1, expanding on the IRv1 structure to support more types of variables and logs suited for 
managing semi-structured logs like JSON. Rather than needing the entire log file in order to 
compress, KV Pair IR can be built one log entry at a time. Like CLP-S, KV Pair IR utilizes a schema 
tree format, but the schema trees use a simplified set of types; string, integer, float, boolean, 
unstructured arrays, and objects. These types can map to multiple CLP-S types, but keeping a 
simplified list of types limits any specialization to the archive. In addition to a schema tree 
built from the user generated structured logs such as JSON, KV Pair IR also supports auto generated 
key value pairs which allows the auto tracking of usueful logging metadata such as timestamp, log 
level, machine info, etc.

### Flattened Representation of Hierarchical Schema Structures

The CLP key-value pair IR format supports arbitrary schema structures, similar to JSON and YAML. To
enhance efficiency, it employs a flattened schema representation derived from [clp-s][clp-s-osdi].
Unlike traditional nested storage, it uses a schema tree to model hierarchical relationships between
keys and values. Each schema tree node has a unique ID and a defined type, representing a key or
container. Rather than storing full schema structures, schemas are encoded as sets of leaf node IDs.
The format serializes kv-pairs as schema-tree-node-ID-value tuples, improving storage efficiency.

### Auto-generated kv-pairs vs. User-generated kv-pairs

KV Pair IR categorizes the kv-pairs of a log event into two categories:
- **Auto-generated kv-pairs**: KV-pairs (e.g., timestamps, log levels, other metadata) that are
  automatically generated by the logging library.
- **User-generated kv-pairs**: Custom kv-pairs (e.g., log messages).

Each KV Pair IR stream maintains auto-generated keys and user-generated keys in two independent
schema trees, ensuring clear differentiation and avoiding unintended interference. This design 
prevents conflicts between auto-generated and user-generated keys by keeping them separate. 
For example if your log message was already maintaining a local timestamp string called "timestamp" 
inside the JSON log message, but you wanted the logging library to track a seperate utc timestamp 
string and call it "timestamp". While you could try to find an unused key name, by keeping the 
automated key in a seperate schema tree, it allows this automated key to have whatever name you 
want and not conflict with the "timestamp" already in use or with any future key that you haven't 
encountered yet.

**NOTE**: CLP-S does not currently support automated key value pairs and will not be refelcted in the CLP-S archive generated from a Key-Value Pair IR Stream containing automated generated keys.

### Building Schema Tree In A Stream

Using the combined [schema trees](#schema-tree), how can we build the schema trees one log event at 
a time to enable a serialized streamable format similar to CLP’s IRv1? We do this by breaking each 
log event down into a series of IR units: schema tree growth IR units describing changes in the 
schema tree and log event IR units which consists of key and value packets. Although we will use 
the terminology IR unit to describe the individual IR stream elements, all of the IR units 
corresponding to a single log event would be sent in a single network packet to preserve the order 
of the individual elements, since the order and location of the IR Units in the stream are used 
inherently by the future log events. 

So the overall format of the IR stream consists of 4 sections; a [magic number](#magic-number) 
which indicates the stream is a KV Pair Stream and how many byte encoding is being used, a 
[metadata IR unit](#metadata) which provides information necessary to properly deserailize the 
stream later, a alternating set of [schema tree growth IR units](#schema-tree-node-insertion-unit) 
and [log event IR units](#log-event-unit), and finally a 
[end of stream IR unit](#end-of-stream-unit) to indicate you've reached the end of a valid KV Pair 
IR Stream. The bulk of the stream will be in spent in that alternating set of schema tree growth IR 
units and log event IR units, one set for each log message. The log event unit contains a set of 
[key and value IR packets](#auto-generated-schema-tree-node-id-value-pairs) for the auto 
generated schema nodes, and then a set of 
[key IR packets and corresponding value IR packets](#user-generated-schema-tree-node-id-value-pairs)
 for each user generated key value pair in the log entry (???). To differnetiate between the auto 
 and user generated node ids we use the 1's complement to encode the auto generated node ids. 
 Rather than organizing the IR units so that each value comes directly after its key, we group the 
 keys and values together. We found that in practice the similarities between keys and between 
 values allows for better compression ratios when stored together. If a log record does not result 
 in any new schema tree nodes, there will be no new schema tree growth IR units for that record. 
 While we stream the IR units we keep the schema tree we’ve built in memory for easy reference, so 
 all packets in the stream can reference the same set of schema tree nodes. 

## Format Specification

### Schema Tree

As described in the [Design Overview](#design-overview), the schema tree efficiently models
hierarchical relationships between keys and values. Each IR stream maintains two merged schema trees
to track encountered schemas in auto-generated and user-generated key-value pairs, respectively.

The schema tree follows these structural assumptions:
- Logical storage in a vector.
  - Index 0 is reserved for the root node. A root node always exists.
  - Nodes are stored in a vector, with node IDs corresponding to their index.
  - New nodes are appended to the end of the vector.
- Type system: Each schema tree node is typed to optimize value representation and retrieval.
    - **Object**: Represents an arbitrary object.
        - As a non-leaf node, it denotes a hierarchical key-value level.
        - As a leaf node, it may represent `null` or an empty key-value set (e.g., `{}`).
    - **Unstructured Array**: Represents an array stored as a JSON string. (Leaf node only)
    - **String**: Represents a UTF-8 encoded byte sequence. (Leaf node only)
    - **Integer**: Represents a 64-bit signed integer. (Leaf node only)
    - **Float**: Represents a double-precision floating-point number. (Leaf node only)
    - **Boolean**: Represents a boolean value. (Leaf node only)
- Key representation: Each schema tree node represents a key, which must be a string.
- Schema tree node locator: Each schema tree node can be uniquely identified by a tuple of:
  - Node type.
  - Parent node ID.
  - Key string.




### IR Stream

//TODO: Explain high level format of IR Stream ... add summary of the 4 parts that make a stream and point to addditional details.

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 4
    A["Magic Number"]:1
    B["Metadata"]:1
    C["Schemas & Log Events"]:1
    D["End"]:1
:::

#### Magic Number

The magic number is a byte sequence to identify a CLP Key-value Pair IR Stream.
- `{0xFD, 0x2F, 0xB5, 0x29}`: Indicating that encoded text ASTs are in four-byte encoding.
- `{0xFD, 0x2F, 0xB5, 0x30}`: Indicating that encoded text ASTs are in eight-byte encoding.

#### Metadata

[Stream-level Metadata](#stream-level-metadata) serialized as a
[JSON Metadata Packet](#json-metadata-packet).

### Stream-level Metadata

Each IR stream contains a stream-level metadata section at the beginning of the stream. The metadata
is represented by key-value pairs.

The mandatory fields:
- `"VERSION"`: The stream version, represented as a string.
- `"VARIABLES_SCHEMA_ID"`: The variable schema ID, represented as a string.
- `"VARIABLE_ENCODING_METHODS_ID"`: The variable encoding ID, represented as a string.

Optional fields:
- `"USER_DEFINED_METADATA"`: Custom metadata provided by the user, represented as key-value pairs.

#### Schemas & Log Events

A variable-length sequence of:
- [Schema Tree Node Insertion Unit](#schema-tree-node-insertion-unit)
- [Log Event Unit](#log-event-unit)

Rules:
- The schema tree starts with only the root node.
- Every schema node referenced in a Log Event Unit must be defined first in a preceding Schema Tree
  Node Insertion Unit within the stream.

#### End

A single [End-of-stream Unit](#end-of-stream-unit) marks the termination of the IR stream.

### IR Units

In this section, we will enumerate all valid IR unites and their formats. Each unit consists of one
or more packets as defined in [IR Packets](#ir-packets).

Supported Units:
- [Schema Tree Node Insertion Unit](#schema-tree-node-insertion-unit)
- [Log Event Unit](#log-event-unit)
- [End-of-stream Unit](#end-of-stream-unit)

#### Schema Tree Node Insertion Unit

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 2
    B["Schema Tree Node Locator Packet"]:2
:::

A Schema Tree Node Insertion Unit signifies the addition of a new schema tree node to a schema tree.
It consists of a single packet, which must be a
[Schema Tree Node Locator Packet](#schema-tree-node-locator-packet).

This IR unit contains the necessary information to:
- Locate a node in a schema tree.
- Determine whether the node should be inserted into the auto-generated or user-generated schema
  tree.

#### Log Event Unit

A Log Event Unit represents a complete log event, including both auto-generated and user-generated
kv-pairs. These kv-pairs are serialized as schema-tree-node-ID-value pairs.

##### Scheme-tree-node-id-value Pairs

A schema-tree-node-ID-value pair consists of:
- An [Encoded Schema Tree Node ID Packet](#encoded-schema-tree-node-id-packet).
- A value packet corresponding to the schema tree node type.

Each schema tree node type supports the following accepted value packets:
- **Object**:
  - [Empty Value Packet](#empty-value-packet)
  - [Null Value Packet](#null-value-packet)
- **Unstructured Array**:
  - [Four-byte Encoded Text AST Packet](#four-byte-encoded-text-ast-packet)
  - [Eight-byte Encoded Text AST Packet](#eight-byte-encoded-text-ast-packet)
- **String**:
  - [String Value Packet](#string-value-packet)
  - [Four-byte Encoded Text AST Packet](#four-byte-encoded-text-ast-packet)
  - [Eight-byte Encoded Text AST Packet](#eight-byte-encoded-text-ast-packet)
- **Integer**:
  - [Integer Value Packet](#integer-value-packet)
- **Float**:
  - [Float Value Packet](#float-value-packet)
- **Boolean**:
  - [True Value Packet](#true-value-packet)
  - [False Value Packet](#false-value-packet)

#### Efficient Serialization of Schema-Tree-Node-ID-Value Pairs

To optimize compression, the auto-generated and user-generated schema-tree-node-ID-value pairs are
structured differently during serialization.

#### Auto Generated Schema Tree Node Id Value Pairs

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
    block-beta
    columns 5
    A["Encoded Node ID #0"]:1
    B["Encoded Value #0"]:1
    C["..."]:1
    D["Encoded Node ID #n"]:1
    E["Encoded Value #n"]:1
:::

Auto-generated schema-tree-node-ID-value pairs are serialized as an array of
`{Encoded Node ID, Encoded Value}` pairs:
- `Encoded Node ID` must belong to the auto-generated schema tree.
- `Encoded Value` must be valid for their respective types.
- If no auto-generated kv-pairs exist, this array is empty.

#### User Generated Schema Tree Node Id Value Pairs

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
columns 6
    A["Encoded Node ID #0"]:1
    B["..."]:1
    C["Encoded Node ID #n"]:1
    D["Encoded Value #0"]:1
    E["..."]:1
    F["Encoded Value #n"]:1
:::

User-generated schema-tree-node-ID-value pairs separate node IDs and values, ensuring the schema
remains a contiguous list of encoded node IDs, which enhances compression efficiency.
- `Encoded Node ID` must belong to the user-generated schema tree.
- `Encoded Value` must be valid for their respective types.
- The number of `Encoded Node ID` must match the number of `Encoded Value`.
- If no user-generated kv-pairs exist, the layout must contain a single
  [Empty Value Packet](#empty-value-packet).

##### Log Event Unit Overall Format Layout

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 2
    A["Auto-generated Schema-tree-node-id-value Pairs"]:1
    B["User-generated Schema-tree-node-id-value Pairs"]:1
:::

A Log Event Unit is serialized in the following order:
1. Auto-generated schema-tree-node-ID-value pairs.
2. User-generated schema-tree-node-ID-value pairs.

#### End-of-stream Unit

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 2
    B["End-of-stream Packet"]:2
:::

An End-of-stream Unit signifies the end of an IR stream. It consists of a single packet, which must
be an [End-of-stream Packet](#end-of-stream-packet).

##### Complete IR Stream

An IR stream must end with this IR unit to be considered complete. If an IR stream does not
terminate with this unit, it is considered incomplete.

### IR Packets

In this section, we will enumerate all valid IR packets and their formats. Each IR packet is
assigned with one or several header bytes that can be uniquely identified. Each header byte will be
given a unique string ID. To find the numerical value of these header bytes, check
[Appendix: IR Packet Header Bytes](#appendix-ir-packet-header-bytes).

- Supported Packets:
  - [Metadata Packet](#metadata-packet)
  - [JSON Metadata Packet](#json-metadata-packet)
  - [Integer Value Packet](#integer-value-packet)
  - [Float Value Packet](#float-value-packet)
  - [True Value Packet](#true-value-packet)
  - [False Value Packet](#false-value-packet)
  - [String Value Packet](#string-value-packet)
  - [Four-byte Encoded Variable Packet](#four-byte-encoded-variable-packet)
  - [Eight-byte Encoded Variable Packet](#eight-byte-encoded-variable-packet)
  - [Dictionary Variable Packet](#dictionary-variable-packet)
  - [Logtype Packet](#logtype-packet)
  - [Four-byte Encoded Text AST Packet](#four-byte-encoded-text-ast-packet)
  - [Eight-byte Encoded Text AST Packet](#eight-byte-encoded-text-ast-packet)
  - [Null Value Packet](#null-value-packet)
  - [Empty Value Packet](#empty-value-packet)
  - [Encoded Schema Tree Node ID Packet](#encoded-schema-tree-node-id-packet)
  - [Encoded Schema Tree Node Parent ID Packet](#encoded-schema-tree-node-parent-id-packet)
  - [Schema Tree Node Locator Packet](#schema-tree-node-locator-packet)
  - [End-of-stream Packet](#end-of-stream-packet)

#### Metadata Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 5
    A["Header Byte"]:1
    B["Length"]:2
    B["Payload: Metadata Byte Sequence"]:2
:::

A Metadata Packet consists of a header byte, an encoded length, and the payload bytes:

- Header Bytes: Specifies the length encoding type.
    - `MetadataLengthUByte`: The length is a 1-byte unsigned integer.
    - `MetadataLengthUShort`: The length is a 2-byte unsigned integer.
- Length: The unsigned integer representing the byte sequence's length, encoded in big-endian format
  as specified by the header.
- Payload: The actual metadata byte sequence.

#### JSON Metadata Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 3
    A["Header Byte"]:1
    B["JSON Metadata"]:2
:::

A JSON Metadata Packet consists of a header byte and the JSON metadata.

- Header Byte: Specifies the metadata encoding type, set to `MetadataJsonEncoding`.
- JSON Metadata: The metadata serialized as a JSON string. Must be a
  [Metadata Packet](#metadata-packet).


#### Integer Value Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 3
    A["Header Byte"]:1
    B["Payload: Encoded Integer"]:2
:::

An Integer Value Packet consists of a header byte and an encoded integer payload:

- Header Byte: Specifies the integer encoding type.
  - `IntValue_1byte`: Payload is a signed 1-byte integer.
  - `IntValue_2byte`: Payload is a signed 2-byte integer.
  - `IntValue_4byte`: Payload is a signed 4-byte integer.
  - `IntValue_8byte`: Payload is a signed 8-byte integer.
- Payload: The integer value, encoded in big-endian format as specified by the header.

#### Float Value Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 3
    A["Header Byte"]:1
    B["Payload: Encoded Float"]:2
:::

A Float Value Packet consists of a header byte and an encoded floating-point payload:

- Header Byte: Indicates the floating-point encoding type.
  - `FloatValue_8byte`: The payload is a double-precision (64-bit) floating-point number.
- Payload: The floating-point value, encoded in big-endian format as specified by the header.

#### True Value Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 1
    A["Header Byte"]:1
:::

A True Value Packet only contains a header byte:

- Header Byte: Represents the boolean value `True`, set to `BoolValue_true`.

#### False Value Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 1
    A["Header Byte"]:1
:::

A False Value Packet only contains a header byte:

- Header Byte: Represents the boolean value `False`, set to `BoolValue_false`.

#### String Value Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 5
    A["Header Byte"]:1
    B["Length"]:2
    C["Payload: String"]:2
:::

A String Value Packet consists of a header byte, an encoded length, and the payload bytes:

- Header Bytes: Specifies the length encoding type.
    - `StringLen_1byte`: The length is a 1-byte unsigned integer.
    - `StringLen_2byte`: The length is a 2-byte unsigned integer.
    - `StringLen_4byte`: The length is a 4-byte unsigned integer.
- Length: The unsigned integer representing the string’s length, encoded in big-endian format as
  specified by the header.
- Payload: The actual string, serialized as a sequence of bytes.

#### Four-byte Encoded Variable Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
columns 3
    A["Header Byte"]:1
    B["Payload: Encoded Variable"]:2
:::

A Four-byte Encoded Variable Packet consists of a header byte and an encoded payload:

- Header Byte: Specifies the encoding type, set to `VarFourByteEncoding`
- Payload: A four-byte sequence encoded in big-endian format.

#### Eight-byte Encoded Variable Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
columns 3
    A["Header Byte"]:1
    B["Payload: Encoded Variable"]:2
:::

An Eight-byte Encoded Variable Packet consists of a header byte and an encoded payload:

- Header Byte: Specifies the encoding type, set to `VarEightByteEncoding`
- Payload: An eight-byte sequence encoded in big-endian format.

#### Dictionary Variable Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 5
    A["Header Byte"]:1
    B["Length"]:2
    C["Payload: String"]:2
:::

A Dictionary Variable Packet consists of a header byte, an encoded length, and the payload bytes:

- Header Bytes: Specifies the length encoding type.
    - `VarStrLenUByte`: The length is a 1-byte unsigned integer.
    - `VarStrLenUShort`: The length is a 2-byte unsigned integer.
    - `VarStrLenInt`: The length is a 4-byte unsigned integer.
- Length: The unsigned integer representing the dictionary variable's length, encoded in big-endian
  format as specified by the header.
- Payload: The actual dictionary variable, serialized as a sequence of bytes.

#### Logtype Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 5
    A["Header Byte"]:1
    B["Length"]:2
    C["Payload: String"]:2
:::

A Logtype Packet consists of a header byte, an encoded length, and the payload bytes:

- Header Bytes: Specifies the length encoding type.
    - `LogtypeStrLenUByte`: The length is a 1-byte unsigned integer.
    - `LogtypeStrLenUShort`: The length is a 2-byte unsigned integer.
    - `LogtypeStrLenInt`: The length is a 4-byte unsigned integer.
- Length: The unsigned integer representing the logtype's length, encoded in big-endian format as
  specified by the header.
- Payload: The actual logtype, serialized as a sequence of bytes.

#### Four-byte Encoded Text AST Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 7
    A["Header Byte"]:1
    B["Variables"]:4
    C["Logtype"]:2
:::

A Four-byte Encoded Text AST Packet consists of a header byte, an array of variable packets, and
a log type packet:

- Header Byte: Specifies the encoding type, set to `StringValue_CLP_4byte`.
- Variables: An array of encoded variables, serialized as variable packets. Each variable packet
  must be one of the following packet type:
  - [Four-byte Encoded Variable Packet](#four-byte-encoded-variable-packet)
  - [Dictionary Variable Packet](#dictionary-variable-packet)
- Logtype: The logtype. Must be a [Logtype Packet](#logtype-packet).

#### Eight-byte Encoded Text AST Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
columns 7
    A["Header Byte"]:1
    B["Variables"]:4
    C["Logtype"]:2
:::

An Eight-byte Encoded Text AST Packet consists of a header byte, an array of variable packets, and
a log type packet:

- Header Byte: Specifies the encoding type, set to `StringValue_CLP_8byte`.
- Variable Packets: An array of encoded variables, serialized as variable packets. Each variable
  packet must be one of the following packet type:
    - [Eight-byte Encoded Variable Packet](#eight-byte-encoded-variable-packet)
    - [Dictionary Variable Packet](#dictionary-variable-packet)
- Logtype Packet: The logtype. Must be a [Logtype Packet](#logtype-packet).

#### Null Value Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 1
    A["Header Byte"]:1
:::

A Null Value Packet only contains a header byte:

- Header Byte: Represents the null value `null`, set to `ObjValue_Null`.

#### Empty Value Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 1
    A["Header Byte"]:1
:::

An Empty Value Packet only contains a header byte:

- Header Byte: Represents the empty value `{}`, set to `ObjValue_Empty`.

#### Encoded Schema Tree Node ID Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 3
    A["Header Byte"]:1
    B["Payload: Encoded Schema Tree Node ID"]:2
:::

An Encoded Schema Tree Node ID Packet consists of a header byte and an encoded schema tree node ID
payload:

- Header Byte: Specifies the schema tree node ID encoding type.
    - `KeyID_1byte`: Payload is a 1-byte one's complement integer.
    - `KeyID_2byte`: Payload is a 2-byte one's complement integer.
    - `KeyID_4byte`: Payload is a 4-byte one's complement integer.
- Payload: The schema tree node ID, encoded in big-endian format as specified by the header.
  - If the encoded ID is positive, it represents a node ID from the user-generated schema tree.
  - If the encoded ID is negative, its absolute value represents a node ID from the auto-generated
    schema tree.

#### Encoded Schema Tree Node Parent ID Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 3
    A["Header Byte"]:1
    B["Payload: Encoded Schema Tree Parent Node ID"]:2
:::

An Encoded Schema Tree Node Parent ID Packet consists of a header byte and an encoded schema tree
node parent ID payload:

- Header Byte: Specifies the schema tree node parent ID encoding type.
    - `ParentID_1byte`: Payload is a 1-byte one's complement integer.
    - `ParentID_2byte`: Payload is a 2-byte one's complement integer.
    - `ParentID_4byte`: Payload is a 4-byte one's complement integer.
- Payload: The schema tree node parent ID, encoded in big-endian format as specified by the header.
    - If the encoded ID is positive, it represents a node ID from the user-generated schema tree.
    - If the encoded ID is negative, its absolute value represents a node ID from the auto-generated
      schema tree.

#### Schema Tree Node Locator Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 5
    A["Header Byte"]:1
    B["Schema Tree Node Parent ID"]:2
    C["Key"]:2
:::

- Header Byte: Specifies the schema tree node's type. Must be one of the following:
  - `SchemaTreeNodeInteger`: Integer type.
  - `SchemaTreeNodeFloat`: Float type.
  - `SchemaTreeNodeBoolean`: Boolean type.
  - `SchemaTreeNodeString`: String type.
  - `SchemaTreeNodeUnstructuredArray`: Unstructured-array type.
  - `SchemaTreeNodeObject`: Object type.
- Schema Tree Node Parent ID: The parent node ID in the schema tree, serialized as an
  [Encoded Schema Tree Node Parent ID Packet](#encoded-schema-tree-node-parent-id-packet).
- Key: The key of the node, serialized as a [String Value Packet](#string-value-packet)

#### End-of-stream Packet

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
    columns 1
    A["Header Byte"]:1
:::

- Header Byte: Represents the end of an IR stream, set to `EndOfStream`.


## Appendix: Definitions

The following terms define key concepts in the context of IR streams:

- **Log event**: A collection of auto-generated and user-generated key-value pairs recorded by an
  application.
- **Encoded text AST**: A logtype paired with a list of encoded variables, representing a structured
  abstraction of a text string.
- **IR encoding**: The process of transforming a log event into an IR log event to achieve specific
  goals, such as improving compressibility or preparing data for ingestion into a CLP/CLP-S archive.
  - Example: Parsing a string into an encoded text AST.
- **IR log event**: A log event after IR encoding.
- **IR decoding**: The reverse process of IR encoding, restoring the original log event.
- **Packet**: A byte stream (which may be part of a larger byte stream) that contains a header and
  an optional payload for storing application data.
- **IR stream serialization**: The process of converting a sequence of IR log events into a byte
  stream following the IR stream protocol.
  - Serialization of an IR unit involves the following steps:
    - **Deconstruction (optional)**: Breaking an IR unit into components that can be individually
      serialized.
    - **Encoding (optional)**: Transforming an IR unit to achieve compression or structural
      efficiency.
      - May require maintaining some global states.
    - **Encapsulation**: Packaging serialized components into packets before writing them to a byte
      stream.
- **Deserialization**: The reverse process of serialization, reconstructing the original IR unit.
  - Deserialization includes:
    - **Unwrapping**: Extracting an IR unit component from a packet.
    - **Decoding**: Reversing any encoding performed during serialization.
    - **Reconstruction**: Assembling components to recover the original IR unit.
- **IR stream serializer**: A class to perform serialization.
- **IR stream deserializer**: A class to perform deserialization.
- **An IR unit**: The smallest meaningful unit of an IR byte stream that a user-level program may
  process.
  - An IR unit may or may not include a packet header.
    - Example: An IR unit may be serialized as a sequence of packets, where each packet contains
      part of the IR unit, as defined by the protocol.


## Appendix: IR Packet Header Bytes

| **Name**                        | **Byte** |
|---------------------------------| -------- |
| SchemaTreeNodeInteger           | 0x71     |
| SchemaTreeNodeFloat             | 0x72     |
| SchemaTreeNodeBoolean           | 0x73     |
| SchemaTreeNodeString            | 0x74     |
| SchemaTreeNodeUnstructuredArray | 0x75     |
| SchemaTreeNodeObject            | 0x76     |
| ParentID_1byte                  | 0x60     |
| ParentID_2byte                  | 0x61     |
| ParentID_4byte                  | 0x62     |
| KeyID_1byte                     | 0x65     |
| KeyID_2byte                     | 0x66     |
| KeyID_4byte                     | 0x67     |
| StringLen_1byte                 | 0x41     |
| StringLen_2byte                 | 0x42     |
| StringLen_4byte                 | 0x43     |
| IntValue_1byte                  | 0x51     |
| IntValue_2byte                  | 0x52     |
| IntValue_4byte                  | 0x53     |
| IntValue_8byte                  | 0x54     |
| IntValue_8byte_unsigned         | 0x55     |
| FloatValue_8byte                | 0x56     |
| BoolValue_true                  | 0x57     |
| BoolValue_false                 | 0x58     |
| StringValue_CLP_4byte           | 0x59     |
| StringValue_CLP_8byte           | 0x5a     |
| ObjValue_Empty                  | 0x5e     |
| ObjValue_Null                   | 0x5f     |
| VarFourByteEncoding             | 0x18     |
| VarEightByteEncoding            | 0x19     |
| VarStrLenUByte                  | 0x11     |
| VarStrLenUShort                 | 0x12     |
| VarStrLenInt                    | 0x13     |
| LogtypeStrLenUByte              | 0x21     |
| LogtypeStrLenUShort             | 0x22     |
| LogtypeStrLenInt                | 0x23     |
| TimestampVar                    | 0x30     |
| TimestampDeltaByte              | 0x31     |
| TimestampDeltaShort             | 0x32     |
| TimestampDeltaInt               | 0x33     |
| TimestampDeltaLong              | 0x34     |
| MetadataJsonEncoding            | 0x01     |
| MetadataLengthUByte             | 0x11     |
| MetadataLengthUShort            | 0x12     |
| EndOfStream                     | 0x00     |

[uber-blog]: https://www.uber.com/en-US/blog/reducing-logging-cost-by-two-orders-of-magnitude-using-clp/
[clp-s-osdi]: https://www.uber.com/en-US/blog/reducing-logging-cost-by-two-orders-of-magnitude-using-clp/

---

## Example
//TODO: Move exmaple to a seperate section and link to corresponding portion of the example after each section

Consider the following two sets of kv-pairs:

<table style="border-collapse: collapse; width: 100%;">
<tr>
    <th style="border: 1px solid black;">Key-Value Pairs #1</th>  
    <th style="border: 1px solid black;">Key-Value Pairs #2</th>  
</tr>
<tr>
    <td style="border: 1px solid black; vertical-align: top;">
        <pre>
{
  "log_id": 2648,
  "version_num": 1.01,
  "has_error": true,
  "error_type": "usage",
  "msg": "UID=0",
  "data": null,
  "input_array": [10, 20, 30],
  "machine_info": {
    "machine_num": 123
  },
  "additional_info": {}
}
        </pre>
    </td>
    <td style="border: 1px solid black; vertical-align: top;">
        <pre>
{
  "log_id": 2649,
  "version_num": 1.01,
  "has_error": false,
  "error_type": "N/A",
  "msg": "success",
  "data": {},
  "input_array": [10, 20, 30],
  "machine_info": {
    "machine_num": 123
  },
  "additional_info": {
    "result": [11, 21, 31]
  }
}
        </pre>
    </td>
</tr>
</table>

The following is a merged schema tree that can represent both sets of kv-pairs:

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
graph LR;
    0["#0:'Root'(Object)"]
    1["#1:'log_id'(Integer)"]
    2["#2:'version_num'(Float)"]
    3["#3:'has_error'(Boolean)"]
    4["#4:'error_type'(String)"]
    5["#5:'msg'(String)"]
    6["#6:'data'(Object)"]
    7["#7:'input_array'(UnstructuredArray)"]
    8["#8:'machine_info'(Object)"]
    9["#9:'machine_num'(Integer)"]
    10["#10:'additional_info'(Object)"]
    11["#11:'msg'(String)"]
    12["#12:'data'(String)"]
    13["#13:'result'(UnstructuredArray)"]
    0 --> 1
    0 --> 2
    0 --> 3
    0 --> 4
    0 --> 5
    0 --> 6
    0 --> 7
    0 --> 8
    0 --> 10
    0 --> 11
    0 --> 12
    8 --> 9
    10 --> 13
:::

Using this schema tree, the kv-pairs are encoded as node-ID sets, representing the structure
compactly:

<table style="border-collapse: collapse; text-align: center; width: 50%;">
<tr>
    <th style="border: 1px solid black;">Key-Value Pairs #1</th>  
    <th style="border: 1px solid black;">Key-Value Pairs #2</th>  
</tr>
<tr>
    <td style="border: 1px solid black;">
        <pre>[1, 2, 3, 4, 5, 6, 7, 9, 10]</pre>
    </td>
    <td style="border: 1px solid black;">
        <pre>[1, 2, 3, 4, 7, 9, 11, 12, 13]</pre>
    </td>
</tr>
</table>

## Putting It Together

Now that we’ve seen all the IR unit types we can look at what the Key-Value Pair IR Stream would 
look like for the two example log events in Figure 1. The stream would start with the schema tree 
growth IR units for Log Event #1. This would consist of growth IR units for all user generated tree
 nodes with IDs 0 - 10 and the auto generated schema node for the timestamp(Figure 12A). After this
  would come the auto genererated key value pair(s) (Figure 12B). Next would come Log Event #1’s 
  user generated keys IR unit group. It will contain a IR unit for each of the user generated 
  schema tree nodes which have values and together with the auto generated keys would make the log 
  events schema (Figure 12C). Completing the IR units corresponding the log event #1 will be the 
  user generated values IR unit group that corresponds to the keys (Figure 12D). This same pattern 
  continues for Log Event #2 completing the serialization of the log events in the KV Pair IR format 
  (Figure 12E-H).

IMAGE PLACEHOLDER
<p style="text-align:center;"><strong>A</strong>: Log Event #1 Schema Tree Growth Packets (PID 
represents Parent ID)</p>

IMAGE PLACEHOLDER
<p style="text-align:center;"><strong>B</strong>:  Log Event #1 Auto Generated Key Value IR Unit 
Pair Group</p>

IMAGE PLACEHOLDER
<p style="text-align:center;"><strong>C</strong>:   Log Event #1 User Generated Key IR Unit Group
</p>

IMAGE PLACEHOLDER
<p style="text-align:center;"><strong>D</strong>:   Log Event #1 User Generated Value IR Unit Group
</p>

IMAGE PLACEHOLDER
<p style="text-align:center;"><strong>E</strong>: Log Event #2 Schema Tree Growth Packets (PID 
represents Parent ID)</p>

IMAGE PLACEHOLDER
<p style="text-align:center;"><strong>F</strong>:  Log Event #2 Auto Generated Key Value IR Unit 
Pair Group</p>

IMAGE PLACEHOLDER
<p style="text-align:center;"><strong>G</strong>:   Log Event #2 User Generated Key IR Unit Group
</p>

IMAGE PLACEHOLDER
<p style="text-align:center;"><strong>H</strong>:   Log Event #2 User Generated Value IR Unit Group
</p>

<p style="text-align:center;"><strong>Figure 12 A-H</strong>: Key-Value Pair IR serialized packet 
stream for example log events in Figure 1</p>

When processing the Key-Value Pair IR Stream into the final CLP-S archive. We can deserialize the 
packet, building the clp-s schema tree based on the schema tree growth packets and use the pre-
parsed key value pairs to build the necessary dictionaries and tables achieving that additional 
compression and deduplication. 

## Check It Out

TODO!!
- Pointers to how to use, etc.

## Background in IRv1

Before we can discuss the Key-Value Pair IR streaming format, let’s talk about IRv1. CLP (Compressed Log 
Processor) is a log compression solution that achieves a higher level of compression than general 
purpose compressors, while maintaining the ability to search the logs without full decompression. 
It manages this by leveraging the unique repetitive characteristics of logs. CLP was originally 
designed for use with unstructured logs. Logs are broken into static text, which remains unchanged 
in different log instances and variables that fill in the blanks in the static text. The building 
of CLPs archives can be broken into two phases. 

Phase 1, is where the logs are parsed and the variables are encoded. This produces an intermediary 
representation (IRv1) which is built one log entry at a time, perfect for processing and partially 
compressing a stream of log entries as they are produced with minimal buffering. 

Phase 2, which can be done offline when resources are available, further deduplicates the logs by 
introducing dictionaries for the static text (i.e. schema) and the variable values. This allows log
 entries to be represented in a table, with each entry consisting a dictionary index of the schema 
 (or static text), a series of dictionary indices representing the values, and an encoded 
 timestamp. Then these tables  can be further compressed using a general compressor like Zstandard 
 in a columnar manner. 

You can read this log post for more details of how CLP's two phase compression works; 
[post](https://www.uber.com/en-CA/blog/reducing-logging-cost-by-two-orders-of-magnitude-using-clp/)
. 
