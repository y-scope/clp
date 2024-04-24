# CLP WebUI

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

## Running in development

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

## Linting

We enforce code quality and consistency across our project using [ESLint][eslint]. Due to specific
dependencies, linting this project requires Node.js v18 or higher. We offer two methods for
performing linting; you may choose either one according to your preference.

### Method 1: Run `Taskfile` tasks

`Taskfile` tasks are available to automatically manage dependency setup and linting operations.

#### Checking for linting errors

```shell
task lint:js-check
```

This will run ESLint on the entire project's source code and report any linting errors.

#### Automatically fixing linting errors

```shell
task lint:js-fix
```

This command attempts to automatically fix any linting issues found in the project.

### Method 2: IDE Integration

To integrate ESLint into IDEs like WebStorm and VSCode, follow these steps:

1. Switch to Node.js v18 or higher
    ```shell
    # Install the latest node if not already installed
    nvm install node

    # Switch to the latest node
    nvm use node
    ```

2. Install the latest ESLint shared config package.
    * We use `--package-lock=false` and `--no-save` to avoid adding the package to
      `package-lock.json` and `package.json`.

    ```shell
    npm --package-lock=false install --no-save eslint-config-yscope@latest
    ```

[eslint]: https://eslint.org/
[nvm]: https://github.com/nvm-sh/nvm
