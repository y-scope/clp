# Single-file archive format

The clp-s single-file archive format is designed to offer high compression and fast search on
dynamically structured log data such as JSON logs. This format is optimized for streaming reads in
order to offer high decompression and search performance for archives stored on object storage
systems such as S3.

This documentation records the details of the single-file archive v0.3.1 format and what it enables
with only minimal discussion of design rationale --- for more information about the design decisions
behind clp-s please refer to [our paper on clp-s][μSlope] or our [blog][s3-blog] on optimizing
clp-s for object storage.

## Format overview

The single-file archive format is divided into the "header", "metadata", and "files" section as
shown in [Figure 1](#figure-1). All archives begin with a 64-byte header that contains important
metadata information such as the archive format version and information needed to read the
"metadata" section. The metadata section is made up of several independent "metadata packets" that
contain archive-level metadata such as the range of timestamp values present in an archive and
information needed to read the "files" section. The files section contains the data structures used
to represent log data and makes up most of the size of an archive; the files section is named as
such because its various components exist as individual files in the multi-file archive format.

:::{note}
This format is little-endian and all fields in the remainder of this document should be treated as
little-endian unless specified otherwise.
:::

(figure-1)=
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
block-beta
  columns 1
  header
  metadata
  block:files_group:1
    columns 1
    files
    block:files_detail
        mpt["/schema_tree"] sm["/schema_ids"] tm["/table_metadata"] vd["/var.dict"] ld["/log.dict"] ad["/array.dict"] t["/0"]
    end
  end
:::
+++
**Figure 1**: High-level single-file archive layout.
::::

## Header section

The archive header is a 64-byte unit at the start of a single-file archive containing some of the
most important metadata information about an archive as shown in [Figure 2](#figure-2). The header
begins with a 4-byte magic number which identifies the file as an archive. The magic number is
followed by a 4-byte version number made up of a 2-byte patch version and 1-byte minor and major 
version numbers respectively. For version 0.X.Y archives every minor version change is breaking and
readers designed for a given minor version are only sometimes backwards compatible in order to
reduce maintenance while the archive format stabilizes.

The compressed archive size is the total size of an archive (including the size of the header). The
original uncompressed size field indicates the total size of the data that was compressed into an
archive. The interpretation of "original uncompressed size" changes somewhat based on what data was
ingested into an archive. For example, when ingesting ndjson the field records the number of bytes
of raw JSON data ingested into an archive and when ingesting KV-IR streams the field records the
number of decompressed bytes of KV-IR ingested into an archive (since KV-IR streams are typically
compressed by a general purposed compressor before being stored).

The metadata section size field indicates the _compressed_ size of the metadata section in bytes.
This field can be used in combination with the uncompressed size field and known header size to
determine the size and offset both the "metadata" and "files" sections.

The compression type for an archive indicates the general purpose compressor used to compress each
section of the archive and is currently one of:
* 0x0000 - ZStandard

All "reserved padding" is reserved for use in future versions of the single-file archive format.

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
    },
    "packet": {
        "bitsPerRow": 8,
        "bitWidth": 128
    }
  }
}%%
packet-beta
  0-3: "Magic Number (0xFD, 0x2F, 0xC5, 0x30)"
  4-5: "Patch Version"
  6: "Minor Version"
  7: "Major Version"
  8-15: "Original Uncompressed Size"
  16-23: "Compressed Archive Size"
  24-55: "Reserved Padding"
  56-59: "Metadata Section Size"
  60-61: "Compression Type"
  62-63: "Reserved Padding"
:::
+++
**Figure 2**: Layout of the 64-byte archive header.
::::

## Metadata section

The metadata section is made up of a compressed sequence of metadata packets with the following
format:
```
[num_packets: uint8_t]
[
  [packet_type: uint8_t]
  [packet_size: uint32_t]
  [packet_content: uint8_t][packet_size]
][num_packets]
```

Each metadata packet has a type, size, and arbitrary binary payload. Different packet types make
different choices for encoding content in this binary payload. Since the size of each packet is
well-defined reader implementations can attempt to read archives containing packet types they are
unfamiliar with by simply skipping the corresponding content. Generally this metadata section design
is intended to allow for some degree of forwards-compatibility and extensibility.

Archives currently support the following metadata packet types, some of which are mandatory:
* 0x00 - ArchiveInfo (mandatory)
* 0x01 - ArchiveFileInfo (mandatory)
* 0x02 - TimestampDictionary
* 0x03 - RangeIndex

### ArchiveInfo packet

### ArchiveFileInfo packet

### TimestampDictionary packet

### RangeIndex packet

## Files section

### Merged parse tree

### Schema map

### Table metadata

### Variable dictionary

### Log-type dictionary

### Array log-type dictionary

### Table segments

:::{warning}
🚧 This section is still under construction.
:::

[s3-blog]: https://blog.yscope.com/optimizing-clp-for-s3-object-storage-b4c502e930ee
[μSlope]: https://www.usenix.org/conference/osdi24/presentation/wang-rui
