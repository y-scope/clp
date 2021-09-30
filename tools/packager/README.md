# CLP Packaging

This repository contains scripts that automatically download CLP's source, compile it, and generate a runnable package.

## Requirements

* An OS capable of running a Linux-based docker
* 10GB of disk space
* At least 2GB of RAM
* An active internet connection
* `docker`, `python3`, `pip`, and `git` pre-installed and available on the user's path
  * `docker` must be runnable [without superuser privileges](https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user)
    (without sudo)
  * For systems with a Python version < 3.7, run `pip3 install -r requirements-pre-3.7.txt`

## Building the package

```bash
pip3 install -r requirements.txt
python3 build-clp-package.py --config ../../config/build-clp-package.yaml
```
The package should be available at `out/<versioned artifact name>.tar.gz`

