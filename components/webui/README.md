# Setup

## Requirements

* Node.js v14 for building and running the webui
  * Meteor.js only [supports](https://docs.meteor.com/install#prereqs-node) Node.js versions >= v10
    and <= v14.
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

[nvm]: https://github.com/nvm-sh/nvm
