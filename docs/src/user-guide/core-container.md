# Core container

To quickly try CLP's core compression, decompression, and search (outside of the package), you can
use the [clp-core-x86-ubuntu-focal][1] container as follows.

Pull the container image:

:::{code-block} shell
docker pull ghcr.io/y-scope/clp/clp-core-x86-ubuntu-focal:main
:::

Start the container with mounts for your logs and output directories:

:::{note}
Be sure to change the paths in the command below before running it.
:::

:::{code-block} shell
docker run \
  --rm \
  -it \
  -u $(id -u):$(id -g) \
  --volume /my/logs/dir:/mnt/logs \
  --volume /my/data/dir:/mnt/data \
  ghcr.io/y-scope/clp/clp-core-x86-ubuntu-focal:main /bin/bash
:::

* Change `/my/logs/dir` to the directory on your machine that contains the logs you wish to
  compress. It will be mounted at `/mnt/logs` in the container.
* Change `/my/data/dir` to the directory on your machine where you want to store the generated
  archives. It will be mounted at `/mnt/data` in the container.

Follow the usage instructions in [clp for unstructured logs](core-unstructured/index) or
[clp for dynamically-structured logs](core-clp-s), depending on the format of your logs.

[1]: https://github.com/y-scope/clp/pkgs/container/clp%2Fclp-core-x86-ubuntu-focal
