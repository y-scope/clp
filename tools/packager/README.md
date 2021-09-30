# CLP Packaging

This repository contains scripts that automatically download CLP's source, compile it, and generate a runnable package.

## Requirements

* An OS capable of running a Linux-based docker
* 10GB of disk space
* Over 2GB of RAM per CPU core
* An active internet connection
* `docker`, `python3`, `pip`, and `git` pre-installed and available on the user's path
  * `docker` must be runnable [without superuser privileges](https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user)
    (without sudo)
  * For systems with a Python version < 3.7, run `pip3 install -r requirements-pre-3.7.txt`

## Building the package

```bash
pip3 install -r requirements.txt
python3 build-clp-package.py
```
The package will be available at `out/clp-package-ubuntu-focal.tar.gz`

## Common build problems

### Not enough memory to build CLP

```bash
compilation terminated.
...
c++: fatal error: Killed signal terminated program cc1plus
```

**Solution:** Set `build-parallelism` in `build-clp-package.yaml` to a value that corresponds to
`<available RAM (GB)> / 2`.