# WebUI

The web interface for the CLP package, which currently consists of a [React] client and a [Fastify]
server. It also serves the [log viewer][yscope-log-viewer].

## Requirements

* Node.js v24 (see `components/webui/.node-version` for the exact version)
* (Optional) [nvm (Node Version Manager)][nvm] to manage different versions of Node.js

## Setup

1. Download the log-viewer's source code:

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

## Configuration

The WebUI's runtime configuration lives in two places under `components/webui/`:

* `settings.json` — the non-secret runtime settings. It has a `public` section (served to the
  browser) and a `server` section (server-only). The browser only ever receives the `public`
  section, so never put secrets here.
* `.env` and `.env.local` — process environment values and secrets. `.env.local` overrides `.env`.
  Secrets stay here, never in `settings.json`.

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

   To customize the host and port the server binds to, or to configure secrets, copy `.env` to
   `.env.local` and update the values there. Settings in `.env.local` override those in `.env`.

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
