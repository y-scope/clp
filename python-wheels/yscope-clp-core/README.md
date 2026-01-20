# CLP core Python Distribution

This project packages the core CLP as a Python distribution, which allows users to install CLP core
cli tools from PyPI:

```
pip install clp-core
```

> [!IMPORTANT]
> The current package only includes `clp-s`. Other CLP core tools will be added in future releases.

> [!IMPORTANT]
> The current package is built inside [manylinux] containers. When running in Debian to use network
> related features, you may see the following errors:
> ```
> 2025-12-05T15:45:57.770-05:00 [error] Encountered curl error while ingesting https://yscope.s3.us-east-2.amazonaws.com/sample-logs/cockroachdb.clp.zst - Code: 77 - Message: error setting certificate verify locations:
> CAfile: /etc/pki/tls/certs/ca-bundle.crt
> CApath: none
> ```
> This is because the CA certificates bundle path is different in Debian-based systems. We will
> address this issue in a future release.

## Documentation

For detailed documentation, check the official CLP core documentation [here][clp-core-docs].


[clp-core-docs]: https://docs.yscope.com/clp/main/user-docs/core-overview.html
[manylinux]: https://github.com/pypa/manylinux
