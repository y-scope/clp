# WebUI

The web interface for the CLP package, which currently consists of a [React] client and a [Fastify]
server. It also serves the [log viewer][yscope-log-viewer].

## Requirements

* Node.js v22 or higher
* (Optional) [nvm (Node Version Manager)][nvm] to manage different versions of Node.js

## Setup

1. Download the yscope-log-viewer's source code:

    ```shell
    task deps:yscope-log-viewer
    ```

2. Install the app's dependencies:

    ```shell
    cd components/webui
    pnpm install
    ```

    If you add a package manually to `package.json` or `package.json` changes for some other reason,
    you should rerun the commands above.

## Running

1. To run both the client and server during development:

    ```shell
    cd components/webui
    pnpm run dev
    ```

2. Or run only the client:

    ```shell
    cd components/webui
    pnpm --filter @webui/client run dev
    ```

3. Or run only the server:

    ```shell
    cd components/webui
    pnpm --filter @webui/server run dev
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
[yscope-log-viewer]: https://github.com/y-scope/yscope-log-viewer
[nvm]: https://github.com/nvm-sh/nvm
[React]: https://reactjs.org/
