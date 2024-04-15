# Setup

## Requirements

* Node.js v14 for building and running the webui
  * Meteor.js only [supports](https://docs.meteor.com/install#prereqs-node) Node.js versions >= v10
    and <= v14.
* Node.js v18 or higher for linting the webui
* (Optional) [nvm (Node Version Manager)][nvm] to manage different versions of Node.js
* [Meteor.js](https://docs.meteor.com/install.html#installation)

## Install the dependencies

```shell
meteor npm install
```

If you ever add a package manually to `package.json` or `package.json` changes
for some other reason, you should rerun this command.

> [!NOTE]
> When running this command, you might see warnings related to uninstalled `eslint-config-yscope`
> peer dependencies, like the ones below:
> ```
> npm WARN eslint-config-yscope@0.0.20 requires a peer of eslint@^8.57.0 but
>  none is installed. You must install peer dependencies yourself.
> ```
> **These `eslint-config-yscope` warnings can be safely ignored.** They occur because the default
> npm version in Node.js v14 does not automatically install peer dependencies. If needed, peer
> dependencies are automatically installed when switching to Node.js v18 or higher for linting
> purposes, as outlined in the [Linting](#linting) section.

# Running in development

The full functionality of the webui depends on other components in the CLP
package:

1. Build the [CLP package](../../docs/Building.md)
2. Start the package: `<clp-package>/sbin/start-clp.sh`
3. Stop the webui instance started by the package: `<clp-package>/sbin/stop-clp.sh webui`
4. Start the webui using meteor (refer to `<clp-package>/etc/clp-config.yml` for the config values):
   ```shell
   MONGO_URL="mongodb://<results_cache.host>:<results_cache.port>/<results_cache.db_name>" \
   ROOT_URL="http://<webui.host>:<webui.port>"                                  \
   CLP_DB_USER="<database.user>"                                                \
   CLP_DB_PASS="<database.password>"                                            \
     meteor --port <webui.port> --settings settings.json
   ```
   
   Here is an example based on the default `clp-config.yml`:
   ```shell
   # Please update `<database.password>` accordingly.
   
   MONGO_URL="mongodb://localhost:27017/clp-search" \
   ROOT_URL="http://localhost:4000"                 \
   CLP_DB_USER="clp-user"                           \
   CLP_DB_PASS="<database.password>"                \
     meteor --port 4000 --settings settings.json
   ```
5. The Web UI should now be available at `http://<webui.host>:<webui.port>`
   (e.g., http://localhost:4000).

# Linting

We enforce code quality and consistency across our project using [ESLint][eslint]. You can use the
following npm scripts defined in `package.json` to check and fix linting issues.

## Setup

Due to specific dependencies, linting this project requires Node.js v18 or higher. Follow these
steps to set up your environment for linting:

1. Switch to Node.js v18 or higher
    
    ```shell
    # Install node v18 if not already installed
    nvm install 18

    # Switch to node v18
    nvm use 18
    ```

2. Re-install the project's dependencies with `--no-package-lock` to prevent `npm` from checking the
   `package-lock.json` file version:

    ```shell
    npm install --no-package-lock
    ```

## Checking for linting errors

To check for linting errors across the project:

```shell
npm run lint
```

This will run ESLint on the entire project's source code and report any linting errors.

## Automatically fixing linting errors

To automatically fix linting errors that can be resolved by ESLint:

```shell
npm run lint:fix
```

This command attempts to automatically fix any linting issues found in the project.

## Linting specific files

If you want to lint a specific file rather than the entire project, you can run ESLint directly with
a custom file path:

```shell
eslint path/to/src.js
```

Replace `path/to/src.js` with the path to the file you want to lint.

[eslint]: https://eslint.org/
[nvm]: https://github.com/nvm-sh/nvm
