# CLP TDL Package

This component contains implementations of CLP tasks that run on Spider. It provides a shared
library that conforms to Spider's Task Definition Language (TDL) package specification.

## Tasks

This component bundles all supported CLP tasks into a single TDL package. The available tasks are
documented below.

### Compression

* `compression::clp_s_s3_compress`: Compress inputs from S3 using `clp-s`.
* `compression::commit`: Commit compression task outcomes to the CLP metadata database.
