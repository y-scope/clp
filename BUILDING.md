# Requirements

* [Meteor](https://docs.meteor.com/install.html#installation)
* [NodeJS 14](https://nodejs.org/download/release/v14.21.3/)
* python3-venv
* [Task](https://taskfile.dev/)

# Building a package

* Run `task` from the root of the project
* The package will be available at `out/clp-package`

NOTE: The taskfile is not set up perfectly so it's possible it may generate an
imperfect build. When this happens, you can delete the `out` directory and build
a fresh package.

# Cleaning up

* Run `task clean` to clean the build of all components
* Run `task clean_package` to clean just the output package
