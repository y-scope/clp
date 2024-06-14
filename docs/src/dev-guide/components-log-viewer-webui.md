# Log Viewer WebUI

A webapp that allows us to serve the [log-viewer] and integrate it with CLP's [webui]. The webapp
currently consists of a [Fastify] server. In the future, we'll add a [React] frontend.

## Requirements

* Node.js v20 or higher

## Setup

Install the app's dependencies:

```shell
cd components/log-viewer-webui/server
npm i
```

## Running

To run the server during development:

```shell
npm run dev
```

To run the server in production:

```shell
npm start
```

In both cases, if you want to customize what host and port the server binds to, you can use the
environment variables in `components/log-viewer-webui/server/.env`.

## Testing

To run all unit tests:

```shell
npm test
```

## Linting

To check for linting errors:

```shell
npm run lint:check
```

To also fix linting errors (if possible):

```shell
npm run lint:fix
```

[Fastify]: https://www.fastify.io/
[log-viewer]: https://github.com/y-scope/yscope-log-viewer
[React]: https://reactjs.org/
[webui]: components-webui.md
