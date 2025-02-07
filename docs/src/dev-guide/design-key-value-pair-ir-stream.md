# Key-Value Pair IR Stream

## Introduction

As outlined in [Uber's blog post][uber-blog] from 2022, the CLP IR stream format is a lightweight
serialization format designed for log streaming compression. However, due to its design constraints,
this IR stream format is primarily suited for simple, unstructured logs, such as raw text logs,
which typically consist of only a timestamp field and a log message field. Consequently,
structured logs, such as JSON logs, cannot be efficiently serialized using this format.

Building upon the principles of the existing IR stream format and drawing inspiration from
[clp-s][clp-s-osdi] (which extends CLP to support structured logs), we have developed a new
IR format: the CLP key-value pair IR stream format. This new format enhances the original design by
efficiently supporting key-value pair serialization, thereby addressing the limitations of the
previous IR format.

This new IR format has been successfully deployed in production environments to serialize real-world
log events for cost reduction. These log events originate from diverse sources, including:
- JSON logs generated on cloud servers.
- Android logs collected from commercial electric vehicles.

## CLP-S && a Motivating example
CLP-S is an extension of the CLP's design principle but applied to semi-structured logs such as 
JSON. It uses the key value structure of JSON entries so that a schema is a unique set of keys and 
the values associated with those keys are the variables. Each schema can be represented by a tree. 
When a unique key name and primitive type pair is encountered, we add a new leaf node to the tree. 
The internal nodes of the tree are any surrounding objects. We merge these individual trees 
together to form a merged parse tree which we’ll refer to as the schema tree. If two keys in 
different schemas share a key name, but not a type, we add that new node to the tree. The schema 
tree formed from the key value pairs directly in the log event we call the user generated schema 
tree.

We can optionally generate second schema tree called the automated generated schema tree. This tree
 can be used for tracking additional metadata about the log event not contained in the log event or
 with keys that may conflict with the log event. For example, if you wanted to maintain an
 additional timestamp or the logging level being used. We can build a schema tree of these auto 
 generated key and value type pairs and combine the auto gnerated schema tree and user generated
 schema tree with a super root node in the CLP-S archive schema tree.

Consider the example log events below and let's say we want to auto generate the current UNIX 
timestamp for this log event. For the first element in log event 1, the schema tree would need to 
contain a leaf node with the name “log_id” and the type integer. In Figure 2, you can see the full 
CLP-S schema tree for the 2 log events. Notice how there are 2 nodes with the names “msg” and 
“data” (nodes 5 and 6 vs nodes 11 and 12), but the types are different. With this merged schema 
tree, each log event can be encoded as a schema which is a series of leaf node IDs and a set of 
values for each of those leaf nodes. You can see the schemas for the two log events in Figure 3. 
Notice for example that node 8 is not listed because it is not a leaf node in either schema and 
node 10 is not listed in schema 1 because it is not a leaf node there. With additional log events, 
each of the schemas in the Schema Map may correspond to more than one log event.  Like in CLP these
 values can be further deduplicated using dictionaries. You can see an overall CLP-S workflow in 
 Figure 4 and you can read further details here; 
 [OSDI24 - wang et. al](https://www.usenix.org/conference/osdi24/presentation/wang-rui). 

<table>
<tr>
<td> <strong>Log Event #1</strong> </td> <td> <strong>Log Event #2</strong> </td>
</tr>
<tr>
<td>
{
  <br>&emsp;“log_id”:&emsp;&emsp;&emsp;&emsp;2648, 
  <br>&emsp;“version_num”:&emsp;1.01,
  <br>&emsp;“has_error”:&emsp;&emsp;&ensp;true,
  <br>&emsp;“error_type”:&emsp;&emsp;“usage”,
  <br>&emsp;“msg”:&emsp;&emsp;&emsp;&emsp;&emsp;“uid=0, CPU usage: 99.99%, \"user_name\"=Yscope”,
  <br>&emsp;“data”:&emsp;&emsp;&emsp;&emsp;&emsp;null,
  <br>&emsp;“input_array”:&emsp;&ensp;[10, 20, 30], 
  <br>&emsp;“machine_info”:&ensp;{
  <br>&emsp;&emsp;&emsp;&emsp;&emsp;“machine_num”:&emsp;123
  <br>&emsp;},
  <br>&emsp;“additional_info”: {}
<br>}
</td>
<td>
{
  <br>&emsp;“log_id”:&emsp;&emsp;&emsp;&emsp;2649,
  <br>&emsp;“version_num”:&emsp;1.01,
  <br>&emsp;“has_error”:&emsp;&emsp;&ensp;false,
  <br>&emsp;“error_type”:&emsp;&emsp;“N\A”,
  <br>&emsp;“msg”:&emsp;&emsp;&emsp;&emsp;&emsp;“success”,
  <br>&emsp;“data”:&emsp;&emsp;&emsp;&emsp;&emsp;{},
  <br>&emsp;“input_array”:&emsp;&ensp;[10, 20, 30], 
  <br>&emsp;“machine_info”:&ensp;{
  <br>&emsp;&emsp;&emsp;&emsp;&emsp;“machine_num”:&emsp;123
  <br>&emsp;},
  <br>&emsp;“additional_info”:&ensp;{
  <br>&emsp;&emsp;&emsp;&emsp;&emsp;“result”:&emsp;[11, 21, 31], 
  <br>&emsp;}
  <br>}
</td>
</tr>
<tr>
<td>Timestamp: 1738254596</td>
<td>Timestamp: 1738256767</td>
</tr>
</table>


<p style="text-align:center;"><strong>Figure 1</strong>: Example Log Events</p>

![image info](./images/design-IRV2_CLPS_Schema_Tree.png)
<p style="text-align:center;"><strong>TODO:: UPDATE WITH AUTO GENERATED NODES</strong>
<strong> Figure 2</strong>: CLP-S Schema Tree</p>

<table style="margin: 0px auto;">
<tr>
<td><strong>Schema ID</strong></td> <td><strong>Nodes in Schema</strong></td>
</tr>
<tr>
<td>0</td>
<td>
[1, 2, 3, 4, <span style="color:red">5</span>, <span style="color:red">6</span>, 7, 9, 
<span style="color:red">10</span>]
</td>
</tr>
<tr>
<td>1</td>
<td>[1, 2, 3, 4, 7, 9, <span style="color:red">11, 12, 13</span>]</td>
</tr>
</table>
<p style="text-align:center;"><strong>TODO:: UPDATE WITH AUTO GENERATED NODES</strong>
<strong> Figure 3</strong>:  CLP-S Schema Map (Nodes unique to a log event are RED)</p>

![image info](./images/design-IRV2_uSlpoe_paper_visualization.png)
<p style="text-align:center;"><strong>Figure 4</strong>: Figure 7 from μSlope: High Compression and
 Fast Search on Semi-Structured Logs by Wang et al.</p>

## Key-Value Pair IR Stream and Streamable IR Units
Our goal for Key-Value Pair IR Stream (KV Pair IR) is to break CLP-S into phases like we did for 
CLP so that we can still achieve a substantial amount of initial compression, but leave the heavy 
resource intensive deduplication of variable values for later processing. Like IRv1, KV Pair IR 
enables the lossless compression of logs on an entry by entry basis. KV Pair IR is a superset of 
IRv1, expanding on the IRv1 structure to support more types of variables and logs suited for 
managing semi-structured logs like JSON. Rather than needing the entire log file in order to 
compress, KV Pair IR can be built one log entry at a time. Like CLP-S, KV Pair IR still maintains 
two schema trees (user generated and auto generated), but the schema trees use a simplified set of 
types string, integer, float, boolean, unstructured arrays, and objects. These types can map to 
multiple CLP-S types, but keeping a simplified list of types limits any specialization to the 
archive. 

Using the example log events from above we can build the simplified schema trees used by KV Pair IR
 in Figure 5 & 6. You can see they share many similarities with the CLP-S schema tree in Figure 2, 
 but uses fewer nodes because nodes like “msg” and “data” are now expressed as generic types.

![image info](./images/design-IRV2_IRv2_Schema_Tree.png)
<p style="text-align:center;"><strong>TODO:: UPDATE WITH AUTO GENERATED NODES</strong>
<strong>Figure 5</strong>: Key-Value Pair IR Stream User Generarted Schema Tree</p>

IMAGE PLACEHOLDER
<p style="text-align:center;"><strong>TODO:: UPDATE WITH AUTO GENERATED NODES</strong> 
<strong>Figure 6</strong>: Key-Value Pair IR Stream Auto Generarted Schema Tree</p>

Using the combined schema trees, how can we build the schema trees one log event at a time to 
enable a streamable format similar to CLP’s IRv1? We do this by breaking each log event down into 
a series of IR units: schema tree growth IR units describing changes in the schema tree, key IR 
Units, and value IR units. Although we will use the terminology IR unit to describe the individual 
IR stream elements, all of the IR units corresponding to a single log event would be sent in a 
single network packet to preserve the order of the individual elements. Since the indices of the IR
 Units in the stream are used inherently by the future log events. 

Each log event will consist of a set of schema tree growth IR units, one for each new tree node 
added, optionally a set of key value IR unit pairs for the auto generated schema nodes, and then a 
set of key IR units and corresponding value IR units corresponding to each user generated key value
 pair in the log entry (Figure 7). To differnetiate between the auto and user generated node ids we
  use the 1's complement to encode the auto generated node ids. You can see this encoding pattern 
  in Figure 8. Rather than organizing the IR units so that each value comes directly after its key,
   we group the keys and values together. We found that in practice the similarities between keys 
   and between values allows for better compression ratios when stored together. If a log record 
   does not result in any new schema tree nodes, there will be no new schema tree growth IR units 
   for that record. While we stream the IR units we keep the schema tree we’ve built in memory for 
   easy reference, so all packets in the stream can reference the same set of schema tree nodes. 

![image info](./images/design-IRV2_Streaming_Structure_for_Each_Log_Entry.svg)
<p style="text-align:center;"><strong>Figure 7</strong>: Streaming Structure for Each Log Entry</p>

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
columns 6
  A1["Schema Tree"]:2
  B1["Auto Generated Key"]:2
  C1["User Generated Key"]:2
  A2["Encoded ID"]:2
  block:group1:2
    columns 4
    B2_1["..."]
    B2_2["-3"]
    B2_3["-2"]
    B2_4["-1"]
  end
  block:group2:2
    columns 4
    C2_1["0"]
    C2_2["1"]
    C2_3["2"]
    C2_4["..."]
  end
  A3["Actual ID"]:2
  block:group3:2
    columns 4
    B3_1["..."]
    B3_2["2"]
    B3_3["1"]
    B3_4["0"]
  end
  block:group4:2
    columns 4
    C3_1["0"]
    C3_2["1"]
    C3_3["2"]
    C3_4["..."]
  end
:::
<p style="text-align:center;"><strong>Figure 8</strong>: Node Id Encoding</p>


:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
  columns 6
    A["Header Byte"]:2
    B["Data"]:4
:::
![image info](./images/design-IRV2_Generic_IR_Unit_Format.svg)
<p style="text-align:center;"><strong>Figure 9</strong>: Generic IR Unit</p>

Every IR unit in the stream will consist of two parts (Figure 9). The first being a header byte 
which will specify what kind of IR unit it is. You can see a list of all the possible header bytes 
in the [Appendix](#appendix). The second being the corresponding data. This data format will change
 depending on the header byte and in some cases will be excluded as all necessary information is 
 specified by the header byte alone. Below we will look at the different kinds of IR units and show
  how to represent the two example log events from Figure 1 using a stream of these IR units. 

Figure 10 shows the structure of the schema tree growth IR unit. The header byte is used to both 
label the IR unit as a growth IR unit and to correspond to the type of node being added to the 
schema tree. Then the data portion is further broken into two parts; the parent schema node ID with
 a byte indicating the size of the integer of the ID (1, 2, 4, or 8 bytes) and the actual node ID 
 of the parent, and the key name of the new node which consists of the number of bytes in the 
 length (1, 2, 4, or 8 bytes), the length of the string, and the characters in the key.

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
  columns 5
    A["Header Byte\n(Node Type)"]:1
    block:group3:2
        columns 1
        B["Parent ID(Int)"]
        block:group5
            D["Len Header"]
            E["Payload"]
        end
    end
    block:group4:2
        columns 1
        C["Key Name (String)"]
        block:group6
            F["Len Header"]
            G["Length"]
            H["Payload"]
        end
    end
:::
![image info](./images/design-IRV2_Schema_Tree_Growth_IR_Unit.png)
<p style="text-align:center;"><strong>Figure 10</strong>: Schema Tree Growth IR Unit</p>

After all the schema tree growth IR units generated by a log event are added to the Key-Value Pair 
IR Stream, then comes all the keys followed by all the values. Figure 11 shows the structure of a 
key IR unit. The header byte identifies it as a key IR unit and indicates the number of bytes of 
the data. Then the data portion consists of the ID of the schema tree node corresponding to the key
 and type of the value that will follow.

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
  columns 6
    A["Header Byte"]:2
    B["Payload: Schema Tree Node ID (1 or 2 Bytes)"]:4
:::
![image info](./images/design-IRV2_Key_IR_Unit.png)
<p style="text-align:center;"><strong>Figure 11</strong>: Key IR Unit</p>

After all the necessary key IR units are streamed the value IR units corresponding to these keys 
follow. There are always the same number of values as there are keys and the values are streamed 
in the same order as the keys. You can see all the types of value IR units in Figure 12.  All value
 IR units contain a header byte that corresponds to their type. In the case of Boolean True, 
 Boolean False, Empty, and Null IR units, we only need the header byte since there is no additional
  information needed. For the remaining IR units the data section varies on the type. For both 
  Integer and Float IR units, the data section is used directly for the encoded numeric value. 
  String IR units are broken into two categories; with and without spaces. If the string does not 
  contain spaces, then the data is split into two parts; the length of the string and the 
  characters of the string. Strings containing spaces are further broken down and encoded using the
   same methodology as CLP. Unstructured arrays are encoded in the same manner as strings with 
   spaces. 

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
  columns 6
    A["Header Byte"]:2
    B["Payload: Encoded Int (4 or 8 Bytes)"]:4
:::
![image info](./images/design-IRV2_Integer_Value_IR_Unit.png)
<p style="text-align:center;"><strong>A</strong>: Integer</p>

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
  columns 6
    A["Header Byte"]:6
:::
![image info](./images/design-IRV2_Boolean_Empty_Object_Null_Value_IR_Unit.png)
<p style="text-align:center;"><strong>B</strong>: Boolean, Empty Object, NULL</p>

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
  columns 6
    A["Header Byte"]:2
    B["Payload: Encoded Float (8 Bytes)"]:4
:::
![image info](./images/design-IRV2_Float_Value_IR_Unit.png)
<p style="text-align:center;"><strong>C</strong>: Float - see uber blog for details of float 
encoding</p>

:::{mermaid}
%%{init: {'theme':'neutral'}}%%
block-beta
  columns 6
    A["Header Byte"]:2
    B["Length"]:2
    c["Payload: Str"]:2
:::
![image info](./images/design-IRV2_NoSpaceString_Value_IR_Unit.png)
<p style="text-align:center;"><strong>D</strong>: Strings 2-ways (Above) No Spaces (Below) With 
Spaces</p>

![image info](./images/design-IRV2_String_Value_IR_Unit.png)
<p style="text-align:center;"><strong>NOTE</strong>: Unstructured arrays use the same IR unit as 
strings with spaces. Also object nodes containing values aren't streamed, because you only stream 
elements with direct value.</p>
<p style="text-align:center;"><strong>Figure 12A-D</strong>: Value IR Units</p>

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

## Appendix

| **Name**                        | **Byte** |
| ------------------------------- | -------- |
| SchemaTreeNodeInteger           | 0x71     |
| SchemaTreeNodeFloat             | 0x72     |
| SchemaTreeNodeBoolean           | 0x73     |
| SchemaTreeNodeString            | 0x74     |
| SchemaTreeNodeUnstructuredArray | 0x75     |
| SchemaTreeNodeObject            | 0x76     |
| ParentID_1byte                  | 0x60     |
| ParentID_2byte                  | 0x61     |
| KeyID_1byte                     | 0x65     |
| KeyID_2byte                     | 0x66     |
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
| EndOfStream                     | 0x00     |

[uber-blog]: https://www.uber.com/en-US/blog/reducing-logging-cost-by-two-orders-of-magnitude-using-clp/
[clp-s-osdi]: https://www.uber.com/en-US/blog/reducing-logging-cost-by-two-orders-of-magnitude-using-clp/
