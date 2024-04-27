# Docs

This directory contains the files necessary to generate a Sphinx-based documentation website for
this project:

* `conf` - Configuration files
* `src` - The actual docs

## Requirements

* [Node.js] >= 16 to be able to [view the output](#viewing-the-output)
* Python 3.10 or later

## Build Commands

* Build the site incrementally:

  ```shell
  task docs:site
  ```

  * The output of the build will be in `../build/docs/html`.

* Clean up the build:

  ```shell
  task docs:clean
  ```

## Viewing the Output

```shell
task docs:serve
```

The command above will install [http-server] and serve the built docs site; `http-server` will print
the address it binds to (usually http://localhost:8080).

[http-server]: https://www.npmjs.com/package/http-server
[Node.js]: https://nodejs.org/en/download/current
