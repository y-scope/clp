# CLP TDL Package

This component contains implementations of CLP tasks that run on Spider. These tasks are built into
a shared library that conforms to the Spider Task Definition Language (TDL) package specification.

## Tasks

This component bundles all supported CLP tasks into a single TDL package. The available tasks are
documented below.

### Compression

* `compression::clp_s_s3_compress`: Compress inputs from S3 using `clp-s`.
* `compression::commit`: Commit compression task outcomes to the CLP metadata database.

### Query

* `query::clp_s_query_to_results_cache`: Query a single `clp-s` archive and write the matching log
  events directly to the results cache.
