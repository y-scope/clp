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
`sbin/admin-tools/archive-manager.sh` allows user to manage their archives on both clp-text and ...
TBD

