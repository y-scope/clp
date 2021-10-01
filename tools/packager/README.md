# Packager

Packager is a tool for generating a runnable CLP package by automatically downloading CLP's source, 
compiling, and bundling it.

## Requirements

* 10GB of disk space
* At least 2GB of RAM
* An active internet connection
* [Docker](https://docs.docker.com/engine/install/)
  * `docker` should be in the user's path, and
  * [runnable without superuser privileges](https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user)
    (without sudo)
* `python3`, `pip`, and `git` pre-installed and available on the user's path
  * For systems with a Python version < 3.7, run `pip3 install -r requirements-pre-3.7.txt`

## Building the package

```bash
pip3 install -r requirements.txt
python3 build-clp-package.py --config ../../config/build-clp-package.yaml
```

* After a successful build, the package will be available at `out/<versioned artifact name>.tar.gz`.
* The package README.md is copied from [package-template](../../components/package-template).


## Troubleshooting

### ModuleNotFoundError

**Error message**: ```ModuleNotFoundError: No module named 'dataclasses'```

**Cause**: When starting the package on some older platforms like Ubuntu 18.04, some required Python modules are not in 
the standard library

**Solution**: `pip install -r requirements-pre-3.7.txt`
