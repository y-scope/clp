# Admin tools

CLP provides a set of scripts that allow system administrators to manage their compressed logs. The
scripts are located in the `sbin/admin-tools/` directory within the CLP package.

Available scripts:

* [archive-manager.sh](#archive-managersh) for listing and deleting archives.
* [dataset-manager.sh](#dataset-managersh) for listing and deleting datasets (only for `clp-json`).

:::{note}
The admin scripts can only be used after CLP starts. For help using CLP, see
[clp-json quick start](../quick-start/clp-json.md) or [clp-text quick start](../quick-start/clp-text.md)
for details.
:::

:::{warning}
The admin scripts **should not** be run while users are compressing or searching logs with CLP.
Doing so may result in undefined behavior.
:::

---

## archive-manager.sh

`sbin/admin-tools/archive-manager.sh` allows users to list and delete archives. For complete usage
information, run:

```bash
sbin/admin-tools/archive-manager.sh --help
```

:::{note}
`clp-json` groups archives into different datasets, whereas `clp-text` currently does not. When
using `archive-manager.sh` with `clp-json`, you should specify a specific dataset using
`--dataset <dataset>`. Otherwise, `archive-manager.sh` will only operate on archives in the dataset
named `default`.
:::

### Examples

1. List all archives:

    ```bash
    sbin/admin-tools/archive-manager.sh find
    ```

2. List only archives that contain log events in a specified time range:

    ```bash
    sbin/admin-tools/archive-manager.sh find \
      --begin-ts <begin-epoch-time-millis> \
      --end-ts <end-epoch-time-millis>
    ```

    * Replace `<begin-epoch-time-millis>` with the timestamp of the time range's beginning (in
      milliseconds since the Unix epoch).
    * Replace `<end-epoch-time-millis>` with the timestamp of time range's end (in milliseconds
      since the Unix epoch).

3. Delete archives by ID:

    ```bash
    sbin/admin-tools/archive-manager.sh del by-ids <archive_id_0> <archive_id_1>
    ```

    * Replace `<archive_id_0>`, `<archive_id_1>`, etc. with the IDs of the archives to delete.

4. Delete archives whose log events fall entirely within a specified time range:

    :::{note}
    Only archives whose log events fall entirely within the specified time range will be deleted.
    Archives whose log events fall outside the specified time range will not be deleted, even if
    those archives also contain log events that fall inside the specified time range.
    :::

    ```bash
    sbin/admin-tools/archive-manager.sh del by-filter \
      --begin-ts <begin-epoch-time-millis> \
      --end-ts <end-epoch-time-millis>
    ```

    * Replace `<begin-epoch-time-millis>` with the timestamp of the time range's beginning (in
      milliseconds since the Unix epoch).
    * Replace `<end-epoch-time-millis>` with the timestamp of time range's end (in milliseconds
      since the Unix epoch).

### Limitations

* `archive-manager.sh` assumes that archive timestamps and the timestamps specified by the user
  are in the **UTC** time zone. (This is because CLP currently doesn't support parsing time zone
  information from log events that have it.) Using the script on archives with non-UTC timestamps
  can lead to an effective time range that is different from the intended value.

  To avoid this issue, you can adjust the given timestamps to account for the offset (in
  milliseconds):

  ```cpp
  adjusted_epoch_timestamp_millis = epoch_timestamp_millis - signed_utc_offset_millis
  ```

* `archive-manager.sh` doesn't support managing archives stored on object storage. This limitation
  will be addressed in a future release.

---

## dataset-manager.sh

`dataset-manager.sh` allows users to list and delete datasets. For complete usage information, run:

```bash
sbin/admin-tools/dataset-manager.sh --help
```

:::{note}
Currently, `dataset-manager.sh` is only supported for `clp-json` (not `clp-text`).
:::

When deleting a dataset, the script removes the dataset's archives and metadata entirely. If you
only want to delete some archives from the dataset, use [archive-manager.sh](#archive-managersh)
instead.

:::{warning}
When deleting a dataset, `dataset-manager.sh` removes all files under the dataset's storage
directory (`key_prefix` in object storage). Therefore, any non-archive files in this storage
directory will also be deleted. Users should avoid storing non-archive files in the archive storage
directory.
:::

### Examples

1. List all datasets:

    ```bash
    sbin/admin-tools/dataset-manager.sh list
    ```

2. Delete dataset by name:

    ```bash
    sbin/admin-tools/dataset-manager.sh del <dataset_0> <dataset_1> ... <dataset_n>
    ```

    * Replace `<dataset_0>`, `<dataset_1>`, etc. with the names of the datasets to delete.
