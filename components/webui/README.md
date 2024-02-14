# Setup

## Requirements

* [Node.js 14](https://nodejs.org/download/release/v14.21.3/) (Meteor.js only
  [supports](https://docs.meteor.com/install#prereqs-node) Node.js versions >= 10 and <= 14)
* [Meteor.js](https://docs.meteor.com/install.html#installation)

## Install the dependencies

```shell
meteor npm install
```

If you ever add a package manually to `package.json` or `package.json` changes
for some other reason, you should rerun this command.

# Running in development

The full functionality of the webui depends on other components in the CLP
package:

1. Build the [CLP package](../../docs/Building.md)
2. Start the package: `<clp-package>/sbin/start-clp.sh`
3. Stop the webui instance started by the package: `<clp-package>/sbin/stop-clp.sh webui`
4. Start the webui using meteor (refer to `<clp-package>/etc/clp-config.yml` for the config values):
   ```shell
   MONGO_URL="mongodb://localhost:<results_cache.port>/<results_cache.db_name>" \
   ROOT_URL="http://<webui.host>"                                               \
   CLP_DB_HOST="<database.host>"                                                \
   CLP_DB_PORT=<database.port>                                                  \
   CLP_DB_NAME="<database.name>"                                                \
   CLP_DB_USER="<database.user>"                                                \
   CLP_DB_PASS="<database.password>"                                            \
     meteor --port <webui.port> --settings settings.json
   ```
   
   Here is an example based on the default `clp-config.yml`:
   ```shell
   # Please update `<database.password>` accordingly
   
   MONGO_URL="mongodb://localhost:27017/clp-search" \
   ROOT_URL="http://localhost"                      \
   CLP_DB_HOST="localhost"                          \
   CLP_DB_PORT=3306                                 \
   CLP_DB_NAME="clp-db"                             \
   CLP_DB_USER="clp-user"                           \
   CLP_DB_PASS="<database.password>"                \
     meteor --port 4000 --settings settings.json
   ```
5. The Web UI should now be available at `http://<webui.host>:<webui.port>`
   (e.g., http://localhost:4000).
