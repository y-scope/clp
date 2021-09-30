# CLP

Compressed Log Processor (CLP) is a tool that compresses text logs and allows users to search the compressed data 
without decompression. CLP's compression ratio is significantly higher than gzip.

## Getting started

CLP can be run in Docker containers, in one of two modes:
* On a single-node (typically for development and testing)
* Across multiple nodes

## Single-node deployment

### Requirements

* [Docker](https://docs.docker.com/engine/install/)
  * `docker` should be in the user's path, and
  * [runnable without superuser privileges](https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user)
    (without sudo)
* Plenty of disk space
* Python3
  * For systems with a version < 3.7, run `pip3 install -r requirements-pre-3.7.txt`

### Starting CLP

```bash
./sbin/start-clp --uncompressed-logs-dir <directory containing your uncompressed logs>
```

Note that running CLP in containers means that the `uncompressed-logs-dir` must be mounted inside the container.
Therefore:
* The `uncompressed-logs-dir` must not include symbolic links to items **outside** of the directory 
* Changing `uncompressed-logs-dir` requires restarting CLP.

### Stopping CLP

```bash
./sbin/stop-clp
```

## Multi-node deployment

### Requirements

* The single-node deployment requirements
* For the scheduler node, port 3306 and 5672 must be available and accessible from all compute nodes
* A distributed file system mounted at the same path on all nodes

### Starting the scheduler

```bash
sbin/start-clp --start-scheduler-only --publish-ports \
  --uncompressed-logs-dir <location of your uncompressed logs on dfs>
```

### Starting the worker(s)

```bash
sbin/start-clp --start-worker-only --publish-ports \
  --uncompressed-logs-dir <location of your uncompressed logs on dfs>
```

### Stopping components

Every component can be stopped by:
```bash
./sbin/stop-clp
```

## Usage

Once CLP is started, you can use it as follows.

### Compressing logs

```bash
./sbin/compress <uncompressed log files/directories>
```

Note:
* The uncompressed logs must be within `uncompressed-logs-dir`
* CLP is designed to compress text logs

For more options, run the script with the `--help` option.

### Decompressing logs

To decompress all compressed logs:
```bash
./sbin/decompress -d <output directory> 
```
For more options, run the script with the `--help` option.

### Searching logs

To search all logs for a given wildcard query:
```bash
./sbin/search <your wildcard query>
```

CLP supports two wildcard characters:
* `*` which matches 0 or more characters
* `?` which matches any single character

For more options, run the script with the `--help` option.

## Common Problems

### ModuleNotFoundError

**Error message**: ```ModuleNotFoundError: No module named 'dataclasses'```

**Cause**: When starting the package on some older platforms like Ubuntu 18.04, some required Python modules are not in 
the standard library

**Solution**: `pip install -r requirements-pre-3.7.txt`


### Difficulties with docker installation
#### Ubuntu 18.04 or 20.04
```shell
sudo apt-get update
sudo apt-get install -y snapd
sudo snap install docker
sudo groupadd docker
sudo usermod -aG docker $USER
newgrp docker
```


