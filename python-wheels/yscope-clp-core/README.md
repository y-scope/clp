# YScope CLP (Compressed Log Processor) Core Python Distribution

This project packages [YScope CLP core][clp-core-docs] as a Python distribution, providing the CLP
core command line tools and a Python binding for performing CLP archive operations programmatically.

## Installation

To install the package:

```shell
pip install yscope-clp-core
```

See the PyPI project page for [release][pypi-release] details.

## Scope and compatibility

> [!IMPORTANT]
> The current package only includes the `clp-s` binary. Support for additional CLP core tools will
> be added in future releases.

## Documentation

For detailed usage of CLP core features and design documentation, see the official CLP documents:

* [**clp-s**][clp-s-docs]: CLP for JSON logs.

[clp-core-docs]: https://docs.yscope.com/clp/main/user-docs/index.html#core
[clp-s-docs]: https://docs.yscope.com/clp/main/user-docs/core-clp-s
[manylinux]: https://github.com/pypa/manylinux
[pypi-release]: https://pypi.org/project/yscope-clp-core
