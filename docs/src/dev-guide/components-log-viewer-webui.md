# Log Viewer WebUI

A webapp that allows us to serve the [log-viewer] and integrate it with CLP's [webui]. The webapp
currently consists of a [React] client and a [Fastify] server.

## Requirements

* Node.js v20 or higher

## Setup

Download the log-viewer's source code:

```bash
task deps:log-viewer
```

Install the app's dependencies:

```shell
cd components/log-viewer-webui
(cd client && npm i)
(cd server && npm i)
```

## Running

To run the client during development:

```shell
npm run start
```

To run the server during development:

```shell
npm run start
```

To run the server in production:

```shell
npm run prod
```

In both cases, if you want to customize what host and port the server binds to, you can use the
environment variables in `components/log-viewer-webui/server/.env`.

## Testing

To run the server's unit tests:

```shell
npm test
```

## Linting

You can lint this component either as part of the entire project or as a standalone component.

### Lint as part of the project

To check for linting errors:

```shell
task lint:check-js
```

To also fix linting errors (if applicable):

```shell
task lint:fix-js
```

### Lint the component alone

To check for linting errors:

```shell
npm run lint:check
```

To also fix linting errors (if applicable):

```shell
npm run lint:fix
```

[Fastify]: https://www.fastify.io/
[log-viewer]: https://github.com/y-scope/yscope-log-viewer
[React]: https://reactjs.org/
[webui]: components-webui.md
