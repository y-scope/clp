# Metadata database

:::{warning}
🚧 This section is still under construction.
:::

## Datasets table

CLP's datasets table describes each dataset stored in CLP and can be used by external tools (e.g.,
a CLP Presto connector) to locate archives that belong to a dataset. [Table 1](#table-1) describes
the high-level schema of the datasets table. Each column is a string with a length that should
accommodate all expected values.

(table-1)=
:::{card}

| Column name               | Type          | Description                                                              |
|---------------------------|---------------|--------------------------------------------------------------------------|
| name                      | VARCHAR(255)  | The *unique* name of the dataset.                                        |
| archive_storage_type      | VARCHAR(64)   | The storage type (e.g., `s3`) where archives are stored.                 |
| archive_storage_directory | VARCHAR(4096) | The directory (on the `archive_storage_type`) where archives are stored. |

+++
**Table 1**: The high-level schema of CLP's datasets table.
:::

:::{note}
CLP typically runs in containers, so when storing archives on a local filesystem, the path it uses
for archives is the path inside the container. However, for external tools to locate these archives,
`archive_storage_directory` must be the path on the host filesystem.
:::
