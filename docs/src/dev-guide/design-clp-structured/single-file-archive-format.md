# Single-file archive format

The clp-s single-file archive format is designed to offer high compression and fast search on
dynamically structured log data such as JSON logs. This format is optimized for streaming reads in
order to offer high decompression and search performance for archives stored on object storage
systems such as S3. 

This documentation records the details of the single-file archive format and what it enables with
only minimal discussion of design rationale --- for more information about the design decisions
behind clp-s please refer to [our paper on clp-s][μSlope] or our [blog][s3-blog] on optimizing
clp-s for object storage.

## Format overview

The single-file archive format is divided into the "header", "metadata", and "files" section as
shown in [Figure 1](#figure-1).

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
        mpt["/schema_tree"] sm["/schema_ids"] c["/table_metadata"] vd["/var.dict"] ld["/log.dict"] ad["/array.dict"] t["/0"]
    end
  end
:::
+++
**Figure 1**: High-level single-file archive layout.
::::

## Header section

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

### ArchiveInfo packet

### ArchiveFileInfo packet

### TimestampDictionary packet

### RangeIndex packet

## Files section

### Merged Parse Tree

:::{warning}
🚧 This section is still under construction.
:::

[s3-blog]: https://blog.yscope.com/optimizing-clp-for-s3-object-storage-b4c502e930ee
[μSlope]: https://www.usenix.org/conference/osdi24/presentation/wang-rui
