# CLP core Python Distribution

This project packages the CLP core as a Python distribution, providing the CLP core command line
tools and a Python binding for performing CLP archive operations programmatically.

## Installation

To install the package:

```shell
pip install yscope-clp-core
```

See the PyPI project page for [release][pypi-release] details.

## Scope and Compatibility

> [!IMPORTANT]
> The current package only includes the `clp-s` binary. Support for additional CLP core tools will
> be added in future releases.

> [!IMPORTANT]
> The package is built inside [manylinux] 2.28 containers to ensure broad Linux compatibility.

## Documentation

For detailed usage of CLP features and design documentation, see the official CLP core
[documentation][clp-core-docs].


[clp-core-docs]: https://docs.yscope.com/clp/main/user-docs/core-overview.html
[manylinux]: https://github.com/pypa/manylinux
[pypi-release]: https://pypi.org/project/yscope-clp-core
