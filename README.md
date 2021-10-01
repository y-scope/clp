# CLP

Compressed Log Processor (CLP) is a tool capable of losslessly compressing text logs and searching 
the compressed logs without decompression. To learn more about it, you can read our 
[paper](https://www.usenix.org/system/files/osdi21-rodrigues.pdf).

## Getting Started

You can download a release from the [releases](TODO) page or you can build the latest by using the
[packager](tools/packager/README.md).

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
* [package-template](components/package-template/src) contains the base directory structure and files of the 
  CLP package.

## Next Steps

This is our open-source release which we will be constantly updating with bug fixes, features, etc.
If you would like a feature or want to report a bug, please file an issue and we'll be happy to engage.
We also welcome any contributions!
