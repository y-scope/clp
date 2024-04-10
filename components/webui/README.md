# Setup

## Requirements

* Node.js
  To manage different versions of Node.js, we recommend using
[nvm (Node Version Manager)](https://github.com/nvm-sh/nvm).
  * **Node.js 14** - For running Meteor. Meteor.js only
    [supports](https://docs.meteor.com/install#prereqs-node) Node.js versions >= 10 and <= 14.
  * **Node.js 18 or higher** - For linting.
* [Meteor.js](https://docs.meteor.com/install.html#installation)

## Install the dependencies

```shell
meteor npm install
```

If you ever add a package manually to `package.json` or `package.json` changes
for some other reason, you should rerun this command.

> ℹ️ When running this command, you might see warnings related to uninstalled `eslint-config-yscope` peer dependencies,
> like the ones below:
> ```
> npm WARN eslint-config-yscope@0.0.20 requires a peer of eslint@^8.57.0 but
>  none is installed. You must install peer dependencies yourself.
> ```
> **These `eslint-config-yscope` warnings can be safely ignored.** They occur because the default npm version in
> Node.js 14 does not automatically install peer dependencies. If needed, peer dependencies are automatically installed
> when switching to Node.js version 18 or higher for linting purposes, as outlined in the [Linting](#linting) section.

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

We enforce code quality and consistency across our project using ESLint. You can use the following
npm scripts defined in `package.json` to check and fix linting issues.

### Preparing Your Environment for Linting

Due to specific dependencies, linting this project requires Node.js version 18 or higher. Follow
these steps to set up your environment for linting:
1. **Switch to Node.js Version 18 or Higher**

    If you have not installed Node.js 18 or higher, use nvm to install it first:
    
    ```shell
    nvm install 18
    ```
    
    Switch to Node.js 18:
    
    ```shell
    nvm use 18
    ```

2. **Re-install Dependencies**

    After switching to the correct Node.js version, re-install the project dependencies:
    
    ```shell
    npm install
    ```

## Checking for Linting Errors

To check for linting errors across the project, run the following command:

```shell
npm run lint
```

This command will run ESLint on the source code, reporting any linting errors without fixing them.

## Automatically Fixing Linting Errors

If you want to automatically fix linting errors that can be resolved by ESLint, use the following
command:

```shell
npm run lint:fix
```

This command attempts to automatically fix any linting issues found in the same
directories.

## Linting Specific Files

While the above commands lint multiple directories, you might sometimes need to lint specific files.
You can do so by running ESLint directly with a custom file path, for example:

```shell
eslint path/to/src.js
```

Replace `path/to/src.js` with the path to the file you want to lint.
