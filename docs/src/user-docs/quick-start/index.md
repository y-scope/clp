# Overview

This guide describes the following:

* [CLP's system requirements](#system-requirements)
* [How to choose a CLP flavor](#choosing-a-flavor)
* [How to use CLP](#using-clp).

---

## System Requirements

To run a CLP release, you'll need:

* [Docker](#docker)
  * `containerd.io` >= 1.7.18
  * `docker-ce` >= 27.0.3
  * `docker-ce-cli` >= 27.0.3
  * `docker-compose-plugin` >= 2.28.1

### Docker

To check whether Docker is installed on your system, run:

```bash
docker version
```

If Docker isn't installed, follow [these instructions][Docker] to install it.

NOTE:

* If you're not running as root, ensure Docker can be run
  [without superuser privileges][docker-non-root].
* If you're using Docker Desktop, ensure version 4.34 or higher is installed.

---

## Choosing a flavor

There are two flavors of CLP:

* **`clp-json`** for compressing and searching **JSON** logs.
* **`clp-text`** for compressing and searching **unstructured text** logs.

:::{note}
Both flavors contain the same binaries but are configured with different values for the
`package.storage_engine` key in the package's config file (`etc/clp-config.yaml`).
:::

Download and extract your chosen flavor from the [Releases][clp-releases] page, and then proceed to
the [appropriate quick-start guide](#using-clp).

If you're having trouble selecting which flavor will work best for you, or you'd like to compare the
capabilities of the two flavors, check out the [clp-text vs. clp-json](./text-v-json.md) page.

---

## Using CLP

Once you've installed CLP's requirements and downloaded a CLP release, proceed to the quick-start
guide for your chosen flavor by clicking the corresponding link below.

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: clp-json
CLP for JSON logs
^^^
How to compress and search JSON logs.
:::

:::{grid-item-card}
:link: clp-text
CLP for unstructured text logs
^^^
How to compress and search unstructured text logs.
:::
::::

[clp-releases]: https://github.com/y-scope/clp/releases
[Docker]: https://docs.docker.com/engine/install/
[docker-non-root]: https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user
