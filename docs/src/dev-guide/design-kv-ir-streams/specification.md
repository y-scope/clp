# Specification

KV-IR streams are structured as shown in [Figure 1](#figure-1). It begins with a magic number,
followed by some metadata, the encoded log events, and finally an end-of-stream marker. Except for
the magic number, the stream is a series of *packets*, where each packet contains:

* a header byte indicating the packet type; and
* an optional payload, potentially containing other packets.

The rest of this page explains the individual components of the stream, highlighting any nontrivial
design choices.

(figure-1)=
::::{card}
:::{mermaid}
block-beta
  columns 1
  
  magicNumber["Magic number"]
  metadata["Metadata"]
  newSchemaNodes["Log events"]
  endOfStream["End of stream"]
:::
+++
**Figure 1:** The high-level structure of a kv-ir stream, with the start of the stream at the top of
the diagram.
::::

## Magic number

The magic number appears first in the stream and is used to identify a file as a kv-ir stream.
There are two possible magic numbers:

* `0xFD2FB529` indicating the stream uses a four-byte encoding for encoded-text AST variables.
* `0xFD2FB530` indicating the stream uses an eight-byte encoding for encoded-text AST variables.
    
:::{warning}
The eight-byte encoding is being deprecated and will be removed in a future release.
:::

## Metadata

The metadata section contains a single [JsonMetadata](#jsonmetadata-packet) packet containing
kv-pairs that describe the contents of the stream. Although JSON can hold values of different types,
the format requires that each key and value must be a string. The kv-pairs in [Table 1](#table-1)
are required while others may be added by applications as necessary.
    
(table-1)=
:::{card}
| Key                           | Value description                                                                                             |
|-------------------------------|---------------------------------------------------------------------------------------------------------------|
| VERSION                       | The stream’s semantic version                                                                                 |
| VARIABLES_SCHEMA_ID           | An identifier for the CLP schema used to parse unstructured text values                                       |
| VARIABLES_ENCODING_METHODS_ID | An identifier for the set of encoding functions used to encode variables parsed from unstructured text values |
+++
**Table 1:** The required kv-pairs in an IR stream's metadata section.
:::

### JsonMetadata packet

* **Header byte:** `0x01`
* **Payload**: A [MetadataUByte](#metadataubyte-packet) packet or
  [MetadataUShort](#metadataushortpacket) packet
  
### MetadataUByte packet

* **Header byte:** `0x11`
* **Payload**:
  * length: uint8_t
  * value: uint8_t\[length\]

### MetadataUShortPacket

* **Header byte:** `0x12`
* **Payload**:
  * length: uint16_t
  * value: uint8_t\[length\]
