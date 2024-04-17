# CLP

Compressed Log Processor (CLP) is a tool that compresses text logs and allows users to search the compressed data
without decompression. CLP's compression ratio is significantly higher than gzip.

## Usage

### Starting CLP

```shell
sbin/start-clp.sh
```

### Compressing logs

```shell
sbin/compress.sh <uncompressed log files/directories>
```

For more options, run the script with the `--help` option.

### Decompressing logs

```shell
sbin/decompress.sh -d <output directory> 
```

For more options, run the script with the `--help` option.

### Searching logs

```bash
sbin/search.sh <your * wildcard * query>
```

CLP supports two wildcard characters:

* `*` which matches 0 or more characters
* `?` which matches any single character

For more options, run the script with the `--help` option.

## Deployment options

CLP can be run in Docker containers, in one of two deployments:

* On a single-node (typically for development and testing)
* Across multiple nodes

## Single-node deployment

### Requirements

* [Docker](https://docs.docker.com/engine/install/)
  * `docker` should be in the user's path
  * `docker` should be [runnable without superuser privileges](https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user)
    (without sudo)
* Python3
  * For systems with a version < 3.7, run `pip3 install -r requirements-pre-3.7.txt`

### Configuration

* If necessary,  you can uncomment and modify any configurations in
  `etc/clp-config.yml`
  * You can use a configuration file at a different location, but you will need
    to pass the location to any CLP command you run
    (`sbin/<command> --config <file path>`).
* To specify the delimiters and variables patterns that should be used to
  compress and search logs, you can place a schema file at `etc/clp-schema.txt`
  * A template schema file with some example patterns is at
    `etc/clp-schema.template.txt`. The syntax of the schema file is described
    [here](https://github.com/y-scope/clp/blob/main/components/core/README-Schema.md).
  * If no schema file is found, CLP uses its default schemas.
* Note: In most cases, changing any configurations will require restarting CLP.

## Multi-node deployment

A multi-node deployment comprises a control node and one or more worker nodes.
Any node can be a control node.

### Requirements

* The single-node deployment requirements
* A distributed filesystem mounted at the same path on all nodes
* Your uncompressed logs should be on the distributed filesystem

### Setup

The easiest way to set up a multi-node deployment is as follows:

* Copy the package to a location on your distributed filesystem
* Modify `etc/clp-config.yml`:
  * Uncomment the file if it is commented.
  * Set `input_logs_directory` to the location of your logs on the distributed
    filesystem.
  * Set the `host` in the `database` section to the hostname/IP of your control
    node.
  * Set the `host` in the `queue` section to the hostname/IP of your control
    node.
* You can now skip to the next sections to see how to start/stop the components.

If you don't want to store data within the package, you can change the
configuration file as follows:

* Set `directory` in the `archive_output` section to a location on the
  distributed filesystem (outside the package).
* Set `data_directory` and `logs_directory` to locations on the distributed
  filesystem (outside the package).

### Starting the control components

On the control node, run the following commands (these must be started in the
order below):

```bash
sbin/start-clp.sh database
sbin/start-clp.sh queue
sbin/start-clp.sh results_cache
sbin/start-clp.sh scheduler
```

### Starting the worker-node components

On every node you want to run workers, run this command:

```bash
sbin/start-clp.sh worker
```

### Stopping components

To stop an individual component on a node, you can use:

```bash
sbin/stop-clp.sh <component name>
```

To stop all components on a node, you can use:

```bash
sbin/stop-clp.sh
```

## Troubleshooting

### ModuleNotFoundError

**Error message**: `ModuleNotFoundError: No module named 'dataclasses'`

**Cause**: When starting the package on some older platforms like Ubuntu 18.04, some required Python modules are not in
the standard library

**Solution**: `pip install -r requirements-pre-3.7.txt`
