To install the dependencies required to build clp-core, follow the steps below.
These same steps are used by our Docker containers.

# Installing dependencies

> [!WARNING]
> Before you run any commands below, you should review the scripts to ensure they will not install
> any dependencies or apply any configurations that you don't expect.

To install all dependencies, run:

```bash
./install-all.sh
```

# Setup dependencies

Enable GCC 10 as the default compiler by running:

```bash
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 10
update-alternatives --set gcc /usr/bin/gcc-10

update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-10 20
update-alternatives --set cc /usr/bin/gcc-10

update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 10
update-alternatives --set g++ /usr/bin/g++-10

update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-10 20
update-alternatives --set c++ /usr/bin/g++-10
```

# Building CLP

* See the instructions in the [main README](../../../../README.md#build)
