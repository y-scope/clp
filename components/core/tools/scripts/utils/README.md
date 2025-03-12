This directory contains uncategorized utility scripts.


* `build-and-run-unit-tests.sh` can be used to build all executables and run
  the unit tests.
* `run-in-container.sh` can be used to run a command (e.g., `make`), in a 
  container containing the core component's dependencies.
  * All commands are run from the root of the core component.
  * This is useful if, for instance, you're developing on Ubuntu X, but the
    package uses Ubuntu Y; you can build in the Ubuntu Y container to avoid
    compatibility issues.
