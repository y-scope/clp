# Single-file archive format

The `clp-s` single-file archive format is designed to offer high compression and fast search on
dynamically-structured log data such as JSON logs. This format is optimized for streaming reads in
order to enable high performance for archives stored on object storage systems such as
[S3][amazon-s3].

This documentation records the details of the single-file archive (v0.3.1) format and what it
enables, with minimal discussion of design rationale. For more information about the design
decisions behind `clp-s`, please refer to [our paper on `clp-s`][μSlope], or our [blog][s3-blog] on
optimizing `clp-s` for object storage.

## Format overview

The single-file archive format is divided into the `header`, `metadata`, and `files` sections, as
shown in [Figure 1](#figure-1). All archives begin with a 64-byte header that contains important
metadata, such as the archive format version and information needed to read the
`metadata` section. The `metadata` section is made up of several independent "metadata packets" that
contain archive-level metadata such as the range of timestamp values present in an archive and
information needed to read the `files` section. The `files` section contains the data structures 
used to represent log data and makes up most of the size of an archive; the `files` section is named
as such because its various components exist as individual files in the multi-file archive format.

:::{note}
The single-file archive format is little-endian. Therefore, all fields in the remainder of this
document should be treated as little-endian unless specified otherwise. Furthermore, any
"struct-like" descriptions of binary formats should be interpreted as _not_ having padding for
alignment, e.g. an `int32_t` followed by an `int64_t` should be interpreted as a 12-byte structure
without padding.
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
        mpt["/schema_tree"] sm["/schema_ids"] tm["/table_metadata"] vd["/var.dict"] ld["/log.dict"]
        ad["/array.dict"] t["/0"]
    end
  end
:::
+++
**Figure 1**: High-level single-file archive layout.
::::

## Header section

The archive header is a 64-byte unit at the start of a single-file archive that contains some of the
most important metadata information about an archive, as shown in [Figure 2](#figure-2). The header
begins with a 4-byte magic number which identifies the file as an archive. The magic number is
followed by a 4-byte version number, comprised of a 2-byte patch version number, followed by 1-byte
minor and major version numbers respectively. For version 0.X.Y archives, every minor version
change is breaking. To reduce maintenance while the archive format stabilizes, not all readers
designed for a given minor version are backwards-compatible.

(figure-2)=
::::{card}
```
Header {
  magic_number: uint8_t[4] = {0xFD, 0x2F, 0xC5, 0x30}
  patch_version: uint16_t
  minor_version: uint8_t
  major_version: uint8_t
  original_uncompressed_size: uint64_t
  compressed_archive_size: uint64_t
  reserved_padding1: uint64_t[4]
  metadata_section_size: uint32_t
  compression_type: uint16_t
  reserved_padding2: uint16_t
}
```
+++
**Figure 2**: Layout of the 64-byte archive header.
::::

The value in `compressed_archive_size` indicates the total size of the archive, including the size
of the header. The `original_uncompressed_size` field indicates the total size of the data that was
compressed into the archive. The interpretation of `original_uncompressed_size` changes somewhat
based on what data was ingested into the archive. For example, when ingesting `ndjson`, the field
records the number of bytes of raw JSON data ingested into the archive --- however, when ingesting
KV-IR streams, the field records the number of decompressed bytes of KV-IR ingested into the archive
(since KV-IR streams are typically compressed by a general purposed compressor before being stored).

The `metadata_section_size` field indicates the _compressed_ size of the metadata section in bytes.
This field can be used in combination with the `original_uncompressed_size` field and known header
size to determine the size and offset of both the `metadata` and `files` sections.

The compression type for an archive indicates the general-purpose compressor used to compress each
section of the archive, and is currently one of:
* `0x0000` - ZStandard

All `reserved_padding` fields are reserved for use in future versions of the single-file archive
format.

## Metadata section

The `metadata` section is made up of a compressed sequence of metadata packets encoded with the
binary format shown in [Figure 3](#figure-3). Metadata packets generally contain information that
relates to the entire archive or large sections of the archive.

(figure-3)=
::::{card}
```
MetadataPacket {
  packet_type: uint8_t
  packet_size: uint32_t
  packet_content: uint8_t[packet_size]
}

MetadataPacketStream {
  num_packets: uint8_t
  packets: MetadataPacket[num_packets]
}
```
+++
**Figure 3**: The `metadata` section packet stream binary format. The `metadata` section is
equivalent to a single instance of `MetadataPacketStream`.
::::

Each metadata packet has a type, size, and arbitrary binary payload. Different packet types make
different choices for encoding content in this binary payload. Since the size of each packet is
well-defined, reader implementations can attempt to read archives containing packet types they are
unfamiliar with by simply skipping the corresponding content. Generally, this metadata section
design is intended to allow for some degree of forwards-compatibility and extensibility.

Archives currently support the following metadata packet types, some of which are optional:
* `0x00` - ArchiveInfo
* `0x01` - ArchiveFileInfo
* `0x02` - TimestampDictionary (optional)
* `0x03` - RangeIndex (optional)

### ArchiveInfo packet

The ArchiveInfo packet is a msgpack map that currently only records the number of segments in an
archive, as shown in [Figure 4](#figure-4). Each archive currently consists of only a single segment
(i.e. there is only one tables segment file), but we still record the number of segments to offer
backwards compatibility if we do start splitting tables into multiple segments.

This metadata packet was originally intended to mimic the "ArchiveMetadata" structure in CLP, but
most of what "ArchiveMetadata" records is now present in either the header or the RangeIndex
metadata packet.

(figure-4)=
::::{card}
```json
{
  "num_segments": 1
}
```
+++
**Figure 4**: Layout of the ArchiveInfo msgpack payload.
::::

### ArchiveFileInfo packet

The ArchiveFileInfo packet is a msgpack map that records the name and offset relative to the start
of the `files` section of each entry in that section, as shown in [Figure 5](#figure-5). Entries
are ordered by their offset into the `files` section. This means that the order of entries in the
ArchiveFileInfo packet corresponds to the order in which the components of the `files` section
should be read to avoid backwards seeks. The offsets stored in this section can be combined with
information from the header to determine the size and absolute offset of every entry in the `files`
section.

(figure-5)=
::::{card}
```json
{
  "files": [
    {
      "n": "/schema_tree",
      "o": 0
    },
    "..."
  ]
}
```
+++
**Figure 5**: Layout of the ArchiveFileInfo msgpack payload. The `"files"` array contains entries
for each file in the `files` section, ordered by their offset. Each file is described by its name
`"n"` and offset into the `files` section `"o"`.
::::

The ArchiveFileInfo packet has entries with the following names in order:
* `"/schema_tree"` - the Merged Parse Tree
* `"/schema_ids"` - the Schema Map
* `"/table_metadata"` - metadata describing the contents of the tables segments
* `"/var.dict"` - the Variable Dictionary
* `"/log.dict"` - the Log-type Dictionary
* `"/array.dict"` - the Array Log-type Dictionary (optional)
* `"/0"` - the first tables segment

The Array Log-type Dictionary is nominally optional for archives that use structured arrays, but in
practice, when structured arrays are enabled, we just store an empty dictionary in this section.

### TimestampDictionary packet

The TimestampDictionary packet is a binary format that records:
1. The key name, corresponding MPT nodes, and range of values for zero or more timestamp columns
2. The format and ID of zero or more timestamp patterns used to encode timestamps in the archive

The details of the binary format can be seen in [Figure 6](#figure-6).

(figure-6)=
::::{card}
```
enum EncodingType : uint64_t {
  Epoch = 0x1,
  DoubleEpoch = 0x2
}

TimestampRange {
  key_len: uint64_t
  key: char[key_len]
  num_column_ids: uint64_t
  column_ids: int32_t[num_column_ids]
  encoding_type: EncodingType
  epoch_start: union {epochtime_t, double}
  epoch_end: union {epochtime_t, double}
}

TimestampPattern {
  pattern_id: uint64_t
  pattern_len: uint64_t
  pattern: char[pattern_len]
}

TimestampDictionary {
  num_ranges: uint64_t
  ranges: TimestampRange[num_entries]
  num_patterns: uint64_t
  patterns: TimestampPattern[num_patterns]
}

```
+++
**Figure 6**: TimestampDictionary binary payload format. The payload is equivalent to a single
instance of "TimestampDictionary".
::::

The key name in each TimestampRange follows the same
[escaping rules](../../user-docs/reference-json-search-syntax.md) we use for key names in KQL
search. Note that we allow each key to map to multiple MPT nodes, and that each key's range can be
recorded as either integer epoch time or double epoch time to handle timestamp columns with
polymorphic types.

The `pattern` in each TimestampPattern entry is a format string that follows the specification from
the `clp_s::TimestampPattern` class. The associated `pattern_id` can be used to uniquely identify
each format string. This allows string timestamps in the archive to be encoded as a tuple of epoch
time and pattern id.

### RangeIndex packet

The archive range index allows users to associate arbitrary properties with each file (or any other
unit of data) ingested into `clp-s`. Collectively, these units of data are referred to as "ingestion
units". Since each archive can potentially aggregate data from multiple files, the archive range
index associates these ingestion unit properties with a logical range of records in an archive.
Note that ingestion units can be split across multiple archives in order to maintain a configured
archive size, and that in these cases the properties become associated with each chunk of the
ingestion unit in each archive.

By default, we automatically store the following properties about each ingestion unit:
* `"_filename"` - the original filename of the ingestion unit as it was passed to `clp-s` during
compression
* `"_file_split_number"` - incremented each time this ingestion-unit is split across another archive
* `"_archive_creator_id"` - UUID associated with a particular _invocation_ of compression

Currently `clp-s` will associate these default properties as well as any properties present in the
metadata section of a KV-IR stream with each ingestion unit. There will likely be future support
for associating arbitrary additional properties with an ingestion unit at compression time.

The RangeIndex packet is encoded as a msgpack array with the format shown in [Figure 7](#figure-7).

(figure-7)=
::::{card}
```json
[
  {
    "s": 0,
    "e": 100,
    "f": {
      "_filename": "/my/file.jsonl",
      "_file_split_number": 0,
      "_archive_creator_id": " 03f2958a-7a2e-448c-a203-60f2cc990d74",
      "arbitrary_user_property": "..."
    }
  },
  "..."
]
```
+++
**Figure 7**: Layout of the RangeIndex msgpack payload. Each entry in the array records a start
index `"s"` and end index `"e"` indicating that the properties correspond to the logical range of
records `[s, e)`. No entries in the range index have overlapping logical ranges, and all entries are
ordered by logical range. The `"f"` object contains the properties associated with an ingestion
unit.
::::

Note that properties created and used by `clp-s` are always prefixed with the `_` character. To
avoid naming collisions with properties created by `clp-s`, users should avoid creating properties
with this prefix.

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
[amazon-s3]: https://aws.amazon.com/s3/
