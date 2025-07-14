# WebUI

The web interface for the CLP package, which currently consists of a [React] client and a [Fastify]
server. It also serves the [log-viewer].

## Requirements

* Node.js v22 or higher
* (Optional) [nvm (Node Version Manager)][nvm] to manage different versions of Node.js

## Setup

1. Download the log-viewer's source code:

    ```shell
    task deps:log-viewer
    ```

2. Install the app's dependencies:

    ```shell
    cd components/webui
    (cd client && npm i)
    (cd server && npm i)
    ```

    If you add a package manually to `package.json` or `package.json` changes for some other reason, 
    you should rerun the commands above.

## Running

1. To run the client during development:

    ```shell
    cd components/webui/client
    npm run start
    ```

2. To run the server during development:

    ```shell
    cd components/webui/server
    npm run dev
    ```

    If you want to customize what host and port the server binds to, you can copy `.env` to
    `.env.local` and modify the values there. The `.env.local` file will override settings in
    `.env`.

## Linting

To check for linting errors:

```shell
task lint:check-js
```

To also fix linting errors (if applicable):

```shell
task lint:fix-js
```

[Fastify]: https://www.fastify.io/
[log-viewer]: https://github.com/y-scope/yscope-log-viewer
[nvm]: https://github.com/nvm-sh/nvm
[React]: https://reactjs.org/
