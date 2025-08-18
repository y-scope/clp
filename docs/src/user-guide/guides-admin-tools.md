# Admin-tools

Admin-tools contain a set of scripts that allow user to manage the logs compressed by CLP, locating
under `sbin/admin-tools/`.
Currently, CLP provides `archive-mananger.sh` and `dataset-manager.sh` that manages at the level of
archives and datasets.

:::{note}
Admin-tools scripts can only be used after CLP starts. To start CLP, see 
[clp-json quick start](quick-start/clp-json.md) or [clp-text quick start](quick-start/clp-text.md) 
for details.
:::

:::{caution}
The admin-tools scripts requires that when running, user will not compress or search logs with CLP. 
Otherwise, the behavior is undefined.
:::

---

## Requirements

* [CLP][clp-releases] v0.4.0 or higher
* [Docker] v28 or higher
* [Docker Compose][docker-compose] v2.20.2 or higher
* Python
* python3-venv (for the version of Python installed)

---

## Archive-manager.sh
`sbin/admin-tools/archive-manager.sh` allows user to list and delete archives on both `clp-text` and 
`clp-json`, with `find` and `del` subcommand.

For example, to list all archives compressed by `clp-test`, run:

```bash
sbin/admin-tools/archive-manager.sh find
```

To narrow the archives to a specific time range:

* Add `--begin-ts <epoch-timestamp-millis>` to filter for archives whose earliest log events is 
after a certain time.
  * `<epoch-timestamp-millis>` is the timestamp as milliseconds since the UNIX epoch.
* Add `--end-ts <epoch-timestamp-millis>` to filter for archives whose most recent log events is 
before a certain time.

:::{caution}
`archive-manager.sh` assumes that archive timestamps are given in **UTC** time. Using the script
on archives with local (i.e., non-UTC) timestamps can lead to an effective time range that is
different from the intended value.

To avoid this issue, either generate logs with UTC timestamps or adjust the 
`<epoch-timestamp-millis>` to account for the offset:
`adjusted_epoch-timestamp-millis = epoch-timestamp-millis - signed_UTC_offset`
:::

To delete archives, user can either specify one or more archive IDs to delete for example:

```bash
sbin/admin-tools/archive-manager.sh del \
  <archive_id_0> \
  <archive_id_1> \
  ... \ 
  <archive_id_n>
```

Alternatively, user can use the time range filtering as `find` command:

```bash
sbin/admin-tools/archive-manager.sh del --begin-ts <epoch-timestamp-millis> 
--end-ts <epoch-timestamp-millis>
```

For more details, refer to the `--help` messages.

:::{note}
`clp-json` groups archives under different datasets. By default, `archive-manager.sh` only lists
and deletes the archive in the `default` dataset. Add `--dataset <dataset>` to manage archives in
a specific dataset.
:::