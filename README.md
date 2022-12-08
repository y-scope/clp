# CLP

Compressed Log Processor (CLP) is a tool capable of losslessly compressing text logs and searching 
the compressed logs without decompression. To learn more about it, you can read our 
[paper](https://www.usenix.org/system/files/osdi21-rodrigues.pdf).

## Getting Started

You can download a release from the [releases](https://github.com/y-scope/clp/releases) page or you can build the latest by using the
[packager](tools/packager/README.md).

For some logs you can use to test CLP, check out our open-source 
[datasets](docs/Datasets.md).

## Project Structure

CLP is currently split across a few different components in the [components](components) 
directory:

* [clp-py-utils](components/clp-py-utils) contains Python utilities common to several of the 
  other components.
* [compression-job-handler](components/compression-job-handler) contains code to submit
  compression jobs to a cluster.
* [core](components/core) contains code to compress uncompressed logs, decompress compressed 
  logs, and search compressed logs.
* [job-orchestration](components/job-orchestration) contains code to schedule compression jobs on
  the cluster.
* [package-template](components/package-template) contains the base directory structure and files of the 
  CLP package.

## Packages
The packages held by this [repository](https://github.com/orgs/y-scope/packages?repo_name=clp) are: 

1. Docker Image `clp/clp-core-dependencies-x86-ubuntu-focal`
    - A docker image containing all the necessary dependencies to build CLP core in an Ubuntu Focal x86 environment
2. Docker Image `clp/clp-core-dependencies-x86-ubuntu-bionic`
    - A docker image containing all the necessary dependencies to build CLP core in an Ubuntu Bionic x86 environment
3. Docker Image `clp/clp-core-dependencies-x86-centos7.4`
    - A docker image containing all the necessary dependencies to build CLP core in a Centos 7.4 x86 environment
4. Docker Image `clp/clp-execution-x86-ubuntu-focal`
    - A docker image containing all the necessary dependencies to run the full CLP package in an x86 environment
5. Docker Image `clp/clp-core-x86-ubuntu-focal`
    - A docker image containing CLP (clp, clg, clo, etc.) binaries built in an Ubuntu Focal x86 environment

## Next Steps

This is our open-source release which we will be constantly updating with bug fixes, features, etc.
If you would like a feature or want to report a bug, please file an issue and we'll be happy to engage.
We also welcome any contributions!
