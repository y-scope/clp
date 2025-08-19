# Admin-tools

Admin-tools is a set of scripts that allow user to manage logs compressed by CLP, located
under `sbin/admin-tools/`.
Currently, CLP provides `archive-mananger.sh` and `dataset-manager.sh`, which manages compressed 
logs at the level of archives and datasets.

:::{note}
Admin-tools scripts can only be used after CLP starts. For help using CLP, see the
[clp-json quick start](quick-start/clp-json.md) or [clp-text quick start](quick-start/clp-text.md)
pages for details.
:::

:::{caution}
When running the admin-tools scripts, users must not compress or search logs with CLP.  
Doing so may result in undefined behavior.
:::

---

## Requirements

* [CLP][clp-releases] v0.4.0 or higher
* [Docker] v28 or higher
* [Docker Compose][docker-compose] v2.20.2 or higher
* Python
* python3-venv (for the version of Python installed)

---

## archive-manager.sh
`sbin/admin-tools/archive-manager.sh` allows users to list and delete archives for both `clp-text` and 
`clp-json`.

For example, to list all archives compressed by `clp-text`, run:

```bash
sbin/admin-tools/archive-manager.sh find
```

To narrow the listed archives to a specific time range:

* Add `--begin-ts <epoch-timestamp-millis>` to the `find` command to filter for archives whose
  earliest log event is after a certain time.
  * `<epoch-timestamp-millis>` is the timestamp as milliseconds since the UNIX epoch.
* Add `--end-ts <epoch-timestamp-millis>` to the `find` command to filter for archives whose most
  recent log event is before a certain time.

To delete archives, users can specify one or more archive IDs, for example:

```bash
sbin/admin-tools/archive-manager.sh del <archive_id_0> <archive_id_1> ... <archive_id_n>
```

Alternatively, users can delete archives using the time range filtering, similar to the `find` command:

```bash
sbin/admin-tools/archive-manager.sh del --begin-ts <epoch-timestamp-millis> \
--end-ts <epoch-timestamp-millis>
```

:::{note}
`clp-json` groups archives into different datasets. By default, `archive-manager.sh` only operates
on the archive in the `default` dataset. Add the `--dataset <dataset>` flag to the commands
described above to manage the archives in a specific dataset.
:::

### Limitations

* `archive-manager.sh` assumes that archive timestamps are given in **UTC** time. Using the script
  on archives with local (i.e., non-UTC) timestamps can lead to an effective time range that is
  different from the intended value.

  To avoid this issue, either generate logs with UTC timestamps or adjust the 
  `<epoch-timestamp-millis>` to account for the offset:

  `adjusted-epoch-timestamp-millis = epoch-timestamp-millis - signed_UTC_offset`


- `archive-manager.sh` does not support managing archives stored on object storage. This limitation
  will be addressed in a future release.

---

## dataset-manager.sh
`dataset-manager.sh` allows users to list and delete datasets for `clp-json`. When deleting a
dataset, the dataset manager removes all compressed archives of the target dataset, as well as all
associated tables in the metadata database.

:::{note}
`dataset-manager.sh` is not supported on `clp-text`, which doesn't have the dataset feature.
:::

To list all existing datasets in the metadata database, run:

```bash
sbin/admin-tools/dataset-manager.sh list
```

To delete dataset(s) by name, run

```bash
sbin/admin-tools/dataset-manager.sh del <dataset_0> <dataset_1> ... <dataset_n>
```

:::{caution}
`dataset-manager.sh` removes files based on the path prefix (or `key_prefix` in object storage). Any
non-archive files in the dataset storage directory will also be deleted. As a precaution, do not
store non-archive files in the archive storage directory or under the archive storage key prefix.
:::

